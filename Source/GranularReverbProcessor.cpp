#include "GranularReverbProcessor.h"
#include "GranularReverbEditor.h"

GranularReverbProcessor::GranularReverbProcessor()
    : juce::AudioProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Params",
            juce::AudioProcessorValueTreeState::ParameterLayout{
                std::make_unique<juce::AudioParameterFloat>(
                    "decay", "Decay", 0.1f, 3.0f, 1.5f),
                std::make_unique<juce::AudioParameterFloat>(
                    "evolve", "Evolve", 0.0f, 1.0f, 0.3f),
                std::make_unique<juce::AudioParameterFloat>(
                    "grainSize", "Grain Size", 20.0f, 500.0f, 150.0f),
                std::make_unique<juce::AudioParameterFloat>(
                    "rate", "Rate", 1.0f, 20.0f, 5.0f),
                std::make_unique<juce::AudioParameterFloat>(
                    "freeze", "Freeze", 0.0f, 1.0f, 0.0f),
                std::make_unique<juce::AudioParameterFloat>(
                    "width", "Width", 0.0f, 1.0f, 0.5f),
                std::make_unique<juce::AudioParameterFloat>(
                    "delay", "Delay", 0.5f, 3.0f, 1.0f),
                std::make_unique<juce::AudioParameterFloat>(
                    "tone", "Tone", 0.0f, 1.0f, 0.5f),
                std::make_unique<juce::AudioParameterFloat>(
                    "dry", "Dry", 0.0f, 1.0f, 0.5f),
                std::make_unique<juce::AudioParameterFloat>(
                    "wet", "Wet", 0.0f, 1.0f, 0.7f)
            })
{
}

void GranularReverbProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    int bufLen = (int)(sampleRate * 0.2);
    for (int ch = 0; ch < 2; ++ch)
    {
        combBuf[ch].clear();
        combBuf[ch].setSize(1, bufLen);
        apBuf[ch].clear();
        apBuf[ch].setSize(1, bufLen);
        preBuf[ch].clear();
        preBuf[ch].setSize(1, (int)(sampleRate * 0.05));
        for (int c = 0; c < NUM_COMBS; ++c)
        {
            combPos[ch][c] = 0;
            combState[ch][c] = 0.0f;
        }
        apPos[ch] = 0;
        apState[ch] = 0.0f;
        prePos[ch] = 0;
    }
    modTimer = 0;
    for (int i = 0; i < NUM_MODS; ++i) mods[i].active = false;
    for (int i = 0; i < NUM_COMBS; ++i) lfoPhase[i] = (float)i * 0.7f;
    toneStateL = 0.0f;
    toneStateR = 0.0f;
    flipTimer = 0;
    flipped = false;
}

void GranularReverbProcessor::processBlock(juce::AudioBuffer<float>& bufferIn,
                                           juce::MidiBuffer&)
{
    auto* decay    = apvts.getRawParameterValue("decay");
    auto* evolve   = apvts.getRawParameterValue("evolve");
    auto* grainSz  = apvts.getRawParameterValue("grainSize");
    auto* rate     = apvts.getRawParameterValue("rate");
    auto* freeze   = apvts.getRawParameterValue("freeze");
    auto* width    = apvts.getRawParameterValue("width");
    auto* delayScl = apvts.getRawParameterValue("delay");
    auto* toneAmt  = apvts.getRawParameterValue("tone");
    auto* dryAmt   = apvts.getRawParameterValue("dry");
    auto* wetAmt   = apvts.getRawParameterValue("wet");

    const int numSamples = bufferIn.getNumSamples();
    const float sr = getSampleRate();
    const float dry = dryAmt->load();
    const float wet = wetAmt->load();
    const bool frozen = freeze->load() > 0.5f;
    const int bufLen = combBuf[0].getNumSamples();
    const int preLen = preBuf[0].getNumSamples();

    // Decay → feedback
    float decaySec = decay->load();
    float baseFb = (float)expf(-0.02f / decaySec);
    if (baseFb > 0.95f) baseFb = 0.95f;
    if (baseFb < 0.1f) baseFb = 0.1f;

    // Damping : 0.2 (brillant) à 0.9 (sombre)
    float baseDamp = 0.2f + 0.7f * (decaySec / 3.0f);

    // Delay scale (0.5x à 3x)
    float dScale = delayScl->load();

    // Délais de base (25-63ms) × scale
    const int baseDelays[NUM_COMBS] = {
        1100, 1350, 1500, 1700, 1900, 2100, 2400, 2700
    };
    const int baseDelaysR[NUM_COMBS] = {
        1200, 1450, 1650, 1850, 2050, 2250, 2550, 2800
    };

    // LFO : profondeur et vitesse modulées par Evolve
    float lfoRate = 0.02f + evolve->load() * 0.3f;
    float lfoDepth = 0.1f + evolve->load() * 0.4f;

    // Allpass
    int apD[4] = {225, 556, 441, 341};
    for (int a = 0; a < 4; ++a)
        if (apD[a] >= bufLen) apD[a] = bufLen / (a + 2);
    float apC = 0.5f;

    // Tone : one-pole lowpass (8kHz à 1.5kHz)
    float toneCutoff = 8000.0f - toneAmt->load() * 6500.0f;
    float toneAlpha = (float)(1.0 - exp(-2.0 * juce::MathConstants<double>::pi * toneCutoff / sr));

    // Stereo flip : tous les ~400ms
    int flipInterval = (int)(sr * 0.4f);

    // Modulation granulaire (fb + damp)
    int interval = (int)(sr / rate->load());
    if (interval < 1) interval = 1;
    if (!frozen && ++modTimer >= interval)
    {
        modTimer = 0;
        for (int m = 0; m < NUM_MODS; ++m)
        {
            if (!mods[m].active)
            {
                mods[m].active = true;
                mods[m].pos = 0.0f;
                float gMs = grainSz->load() * (0.5f + juce::Random::getSystemRandom().nextFloat());
                mods[m].length = gMs * sr / 1000.0f;
                if (mods[m].length < 50.0f) mods[m].length = 50.0f;
                // FB : ±10% autour de baseFb (toujours en dessous)
                mods[m].targetFb = baseFb * (0.85f + 0.1f * juce::Random::getSystemRandom().nextFloat());
                mods[m].startFb = baseFb;
                // Damp : ±20% autour de baseDamp
                float dampVar = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * 0.2f;
                mods[m].targetDamp = baseDamp * (1.0f + dampVar);
                if (mods[m].targetDamp < 0.1f) mods[m].targetDamp = 0.1f;
                if (mods[m].targetDamp > 0.95f) mods[m].targetDamp = 0.95f;
                mods[m].startDamp = baseDamp;
                break;
            }
        }
    }

    float modFbSum = 0.0f, modDampSum = 0.0f, modWin = 0.0f;
    for (int m = 0; m < NUM_MODS; ++m)
    {
        auto& mo = mods[m];
        if (!mo.active) continue;
        if (mo.pos >= mo.length) { mo.active = false; continue; }
        float t = mo.pos / mo.length;
        float win = 0.5f * (1.0f - cosf(juce::MathConstants<float>::pi * t));
        float fbVal = mo.startFb + (mo.targetFb - mo.startFb) * win;
        float dampVal = mo.startDamp + (mo.targetDamp - mo.startDamp) * win;
        modFbSum += fbVal * win;
        modDampSum += dampVal * win;
        modWin += win;
        mo.pos += 1.0f;
    }
    float fb = (modWin > 0.01f) ? (modFbSum / modWin) : baseFb;
    float damp = (modWin > 0.01f) ? (modDampSum / modWin) : baseDamp;
    if (fb > 0.95f) fb = 0.95f;
    if (fb < 0.0f) fb = 0.0f;
    if (damp > 0.95f) damp = 0.95f;
    if (damp < 0.1f) damp = 0.1f;

    // Asymétrie L/R (decay différent)
    float fbL = fb;
    float fbR = fb * (1.0f - width->load() * 0.08f);
    if (fbR > 0.95f) fbR = 0.95f;

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = bufferIn.getSample(0, i);
        float inR = bufferIn.getSample(1, i);

        // Stereo flip
        if (++flipTimer >= flipInterval)
        {
            flipTimer = 0;
            flipped = !flipped;
        }
        float swL = inL, swR = inR;
        if (flipped) { swL = inR; swR = inL; }

        // Avancer les LFO
        for (int c = 0; c < NUM_COMBS; ++c)
        {
            lfoPhase[c] += 2.0f * juce::MathConstants<float>::pi * lfoRate / (float)sr;
            if (lfoPhase[c] > 2.0f * juce::MathConstants<float>::pi)
                lfoPhase[c] -= 2.0f * juce::MathConstants<float>::pi;
        }

        // PRE-DELAY (buffer circulaire 5ms max)
        {
            float* buf = preBuf[0].getWritePointer(0);
            int read = (prePos[0] - 1 + preLen * 4) % preLen;
            buf[prePos[0]] = swL;
            swL = buf[read];
            prePos[0] = (prePos[0] + 1) % preLen;
        }
        {
            float* buf = preBuf[1].getWritePointer(0);
            int read = (prePos[1] - 1 + preLen * 4) % preLen;
            buf[prePos[1]] = swR;
            swR = buf[read];
            prePos[1] = (prePos[1] + 1) % preLen;
        }

        // ALLPASS ENTRÉE (L)
        for (int a = 0; a < 2; ++a)
        {
            float* buf = apBuf[0].getWritePointer(0);
            int d = apD[a];
            int read = (apPos[0] - d + bufLen * 4) % bufLen;
            float prev = buf[read];
            float y = -swL + prev + apC * (apState[0] - prev);
            buf[apPos[0]] = y;
            apState[0] = y;
            apPos[0] = (apPos[0] + 1) % bufLen;
            swL = y;
        }
        // ALLPASS ENTRÉE (R)
        for (int a = 0; a < 2; ++a)
        {
            float* buf = apBuf[1].getWritePointer(0);
            int d = apD[a];
            int read = (apPos[1] - d + bufLen * 4) % bufLen;
            float prev = buf[read];
            float y = -swR + prev + apC * (apState[1] - prev);
            buf[apPos[1]] = y;
            apState[1] = y;
            apPos[1] = (apPos[1] + 1) % bufLen;
            swR = y;
        }

        // 8 COMBS (L)
        float combOutL = 0.0f;
        for (int c = 0; c < NUM_COMBS; ++c)
        {
            float lfoVal = sinf(lfoPhase[c]);
            int modDelay = (int)(baseDelays[c] * dScale * (1.0f + lfoDepth * lfoVal));
            if (modDelay < 10) modDelay = 10;
            if (modDelay >= bufLen) modDelay = bufLen - 10;

            float* buf = combBuf[0].getWritePointer(0);
            int read = (combPos[0][c] - modDelay + bufLen * 4) % bufLen;
            float fbSample = buf[read];
            float damped = combState[0][c] * (1.0f - damp) + fbSample * damp;
            combState[0][c] = damped;
            float y = swL + fbL * damped;
            buf[combPos[0][c]] = y;
            combPos[0][c] = (combPos[0][c] + 1) % bufLen;
            combOutL += y;
        }

        // 8 COMBS (R)
        float combOutR = 0.0f;
        for (int c = 0; c < NUM_COMBS; ++c)
        {
            float lfoVal = sinf(lfoPhase[c] + 0.5f);
            int modDelay = (int)(baseDelaysR[c] * dScale * (1.0f + lfoDepth * lfoVal));
            if (modDelay < 10) modDelay = 10;
            if (modDelay >= bufLen) modDelay = bufLen - 10;

            float* buf = combBuf[1].getWritePointer(0);
            int read = (combPos[1][c] - modDelay + bufLen * 4) % bufLen;
            float fbSample = buf[read];
            float damped = combState[1][c] * (1.0f - damp) + fbSample * damp;
            combState[1][c] = damped;
            float y = swR + fbR * damped;
            buf[combPos[1][c]] = y;
            combPos[1][c] = (combPos[1][c] + 1) % bufLen;
            combOutR += y;
        }

        combOutL *= 0.125f;
        combOutR *= 0.125f;

        // ALLPASS SORTIE (L)
        for (int a = 0; a < 2; ++a)
        {
            float* buf = apBuf[0].getWritePointer(0);
            int d = apD[a + 2];
            int read = (apPos[0] - d + bufLen * 4) % bufLen;
            float prev = buf[read];
            float y = -combOutL + prev + apC * (apState[0] - prev);
            buf[apPos[0]] = y;
            apState[0] = y;
            apPos[0] = (apPos[0] + 1) % bufLen;
            combOutL = y;
        }
        // ALLPASS SORTIE (R)
        for (int a = 0; a < 2; ++a)
        {
            float* buf = apBuf[1].getWritePointer(0);
            int d = apD[a + 2];
            int read = (apPos[1] - d + bufLen * 4) % bufLen;
            float prev = buf[read];
            float y = -combOutR + prev + apC * (apState[1] - prev);
            buf[apPos[1]] = y;
            apState[1] = y;
            apPos[1] = (apPos[1] + 1) % bufLen;
            combOutR = y;
        }

        // Crossfeed
        float cross = width->load() * 0.3f;
        float outL = combOutL + combOutR * cross;
        float outR = combOutR + combOutL * cross;

        // TONE : one-pole lowpass sur wet
        toneStateL += toneAlpha * (outL - toneStateL);
        toneStateR += toneAlpha * (outR - toneStateR);
        outL = toneStateL;
        outR = toneStateR;

        // Gain
        outL *= 4.0f;
        outR *= 4.0f;

        // Soft-clip
        outL = tanhf(outL * 2.0f) / tanhf(2.0f);
        outR = tanhf(outR * 2.0f) / tanhf(2.0f);

        bufferIn.setSample(0, i, inL * dry + outL * wet);
        bufferIn.setSample(1, i, inR * dry + outR * wet);
    }
}

juce::AudioProcessorEditor* GranularReverbProcessor::createEditor()
{
    return new GranularReverbEditor(*this);
}

juce::AudioProcessor* createPluginFilter()
{
    return new GranularReverbProcessor();
}
