#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class GranularReverbProcessor : public juce::AudioProcessor
{
public:
    GranularReverbProcessor();
    ~GranularReverbProcessor() override = default;

    void prepareToPlay(double sampleRate, int blockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Granular Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_MODS = 8;

    struct Mod {
        bool active = false;
        float pos = 0.0f;
        float length = 0.0f;
        float targetFb = 0.0f;
        float targetDamp = 0.0f;
        float startFb = 0.0f;
        float startDamp = 0.0f;
    };

    juce::AudioBuffer<float> combBuf[2];
    int combPos[2][NUM_COMBS] = {};
    float combState[2][NUM_COMBS] = {};

    juce::AudioBuffer<float> apBuf[2];
    int apPos[2] = {};
    float apState[2] = {};

    // Pre-delay buffer
    juce::AudioBuffer<float> preBuf[2];
    int prePos[2] = {};

    // Tone (one-pole lowpass)
    float toneStateL = 0.0f;
    float toneStateR = 0.0f;

    Mod mods[NUM_MODS];
    int modTimer = 0;
    float lfoPhase[NUM_COMBS] = {};
    int flipTimer = 0;
    bool flipped = false;

    JUCE_DECLARE_NON_COPYABLE(GranularReverbProcessor)
};
