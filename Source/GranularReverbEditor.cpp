#include "GranularReverbEditor.h"

// ─── Palette R3Z0 ──────────────────────────────────────────────
const juce::Colour GranularReverbEditor::bg      { 0xff0f0d1a };
const juce::Colour GranularReverbEditor::panel   { 0xff1a1630 };
const juce::Colour GranularReverbEditor::textMain{ 0xffe8e0d0 };
const juce::Colour GranularReverbEditor::gold    { 0xffb58900 };
const juce::Colour GranularReverbEditor::purple  { 0xff4f44e3 };
const juce::Colour GranularReverbEditor::track   { 0xff252040 };
const juce::Colour GranularReverbEditor::line    { 0xff3a3555 };
const juce::Colour GranularReverbEditor::ledOn   { 0xffb58900 };
const juce::Colour GranularReverbEditor::ledOff  { 0xff252040 };

static const char* presetNames[] = {
    "Custom", "Tight", "Room", "Hall", "Frozen", "Drums", "Cloud", "Dark",
    "Slap", "Wide", "Echo", "Snare", "Kit", "Gated"
};

//           decay  evolve  grain  rate  frz  width  delay  tone  preDly  asym  dry   wet
static const float presetVals[][12] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    { 0.2f, 0.1f,  60.0f, 14.0f, 0.0f, 0.2f, 0.7f, 0.15f,   0.0f, 1.0f, 0.8f, 0.4f},
    { 0.8f, 0.3f, 150.0f,  6.0f, 0.0f, 0.5f, 1.0f, 0.4f,  10.0f, 1.0f, 0.6f, 0.6f},
    { 2.0f, 0.6f, 250.0f,  3.0f, 0.0f, 0.8f, 1.5f, 0.6f,  20.0f, 1.0f, 0.4f, 0.8f},
    { 2.5f, 0.2f, 350.0f,  2.0f, 1.0f, 0.7f, 1.2f, 0.5f,  10.0f, 1.0f, 0.3f, 0.9f},
    { 0.1f, 0.6f,  40.0f, 18.0f, 0.0f, 0.1f, 0.5f, 0.0f,   0.0f, 0.0f, 0.9f, 0.3f},
    { 3.0f, 0.8f, 450.0f,  1.0f, 0.0f, 0.9f, 2.5f, 0.8f,  30.0f, 1.0f, 0.2f, 0.9f},
    { 1.5f, 0.4f, 200.0f,  4.0f, 0.0f, 0.6f, 2.0f, 0.85f, 15.0f, 1.0f, 0.4f, 0.8f},
    { 0.35f, 0.2f,  80.0f,  8.0f, 0.0f, 0.3f, 0.8f, 0.2f,  60.0f, 1.0f, 0.7f, 0.5f},
    { 1.2f, 0.3f, 180.0f,  4.0f, 0.0f, 1.0f, 1.5f, 0.4f,  10.0f, 0.0f, 0.5f, 0.7f},
    { 0.3f, 0.1f,  30.0f, 14.0f, 0.0f, 0.1f, 0.5f, 0.0f,  80.0f, 0.0f, 0.6f, 0.4f},
    { 0.15f, 0.2f,  50.0f, 12.0f, 0.0f, 0.15f, 0.6f, 0.1f,  0.0f, 0.0f, 0.85f, 0.35f},
    { 0.25f, 0.3f,  45.0f, 15.0f, 0.0f, 0.2f,  0.5f, 0.05f, 0.0f, 0.0f, 0.9f, 0.3f},
    { 0.08f, 0.4f,  35.0f, 20.0f, 0.3f, 0.1f, 0.5f, 0.0f,  0.0f, 0.0f, 0.95f, 0.25f},
};

static const char* paramNames[9] = {
    "Decay", "Evolve", "Grain", "Rate", "Freeze", "Width", "Delay", "Tone", "Pre-Dly"
};

static juce::FontOptions makeFont(float size)
{
    return juce::FontOptions("Inter", size, juce::Font::plain);
}

static juce::FontOptions makeFontBold(float size)
{
    return juce::FontOptions("Inter", size, juce::Font::bold);
}

// ─── Layout ────────────────────────────────────────────────────
static constexpr int W         = 380;
static constexpr int MARGIN    = 18;
static constexpr int HDR_H     = 50;
static constexpr int PRESET_Y  = 56;
static constexpr int PRESET_H  = 30;
static constexpr int ROW_START = 100;
static constexpr int ROW_H     = 42;
static constexpr int SLIDER_X  = 120;
static constexpr int SLIDER_W  = 200;
static constexpr int SLIDER_H  = 8;
static constexpr int VAL_X     = 328;
static constexpr int VAL_W     = 42;
static constexpr int STOP_W    = 56;
static constexpr int STOP_H    = 24;

GranularReverbEditor::GranularReverbEditor(GranularReverbProcessor& p)
    : juce::AudioProcessorEditor(&p),
      aDecay(p.apvts, "decay", sDecay),
      aEvolve(p.apvts, "evolve", sEvolve),
      aGrain(p.apvts, "grainSize", sGrain),
      aRate(p.apvts, "rate", sRate),
      aFrz(p.apvts, "freeze", sFrz),
      aWidth(p.apvts, "width", sWidth),
      aDelay(p.apvts, "delay", sDelay),
      aTone(p.apvts, "tone", sTone),
      aPreDelay(p.apvts, "preDelay", sPreDelay),
      aAsym(p.apvts, "asym", asymBtn),
      aDry(p.apvts, "dry", sDry),
      aWet(p.apvts, "wet", sWet),
      proc(p)
{
    for (int i = 0; i < 14; ++i)
        presetBox.addItem(presetNames[i], i + 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.setBounds(MARGIN, PRESET_Y, W - 2 * MARGIN, PRESET_H);
    presetBox.onChange = [this] { applyPreset(presetBox.getSelectedId() - 1); };
    addAndMakeVisible(presetBox);

    juce::Slider* main[9] = {&sDecay, &sEvolve, &sGrain, &sRate, &sFrz, &sWidth, &sDelay, &sTone, &sPreDelay};
    for (int i = 0; i < 9; ++i)
    {
        main[i]->setSliderStyle(juce::Slider::LinearHorizontal);
        main[i]->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        main[i]->setBounds(SLIDER_X, ROW_START + i * ROW_H + 12, SLIDER_W, 20);
        main[i]->setTooltip(paramNames[i]);
        main[i]->onValueChange = [this] { repaint(); };
        addAndMakeVisible(main[i]);
    }

    int yAsym = ROW_START + 9 * ROW_H + 4;
    asymBtn.setButtonText("");
    asymBtn.setBounds(MARGIN, yAsym, 18, 18);
    asymBtn.setClickingTogglesState(true);
    addAndMakeVisible(asymBtn);

    int yDry = yAsym + 30;
    int yWet = yDry + 38;

    sDry.setSliderStyle(juce::Slider::LinearHorizontal);
    sDry.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sDry.setBounds(SLIDER_X, yDry + 12, SLIDER_W, 20);
    sDry.setTooltip("Dry");
    sDry.onValueChange = [this] { repaint(); };
    addAndMakeVisible(sDry);

    sWet.setSliderStyle(juce::Slider::LinearHorizontal);
    sWet.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sWet.setBounds(SLIDER_X, yWet + 12, SLIDER_W, 20);
    sWet.setTooltip("Wet");
    sWet.onValueChange = [this] { repaint(); };
    addAndMakeVisible(sWet);

    int totalH = yWet + 42;
    setSize(W, totalH);
}

void GranularReverbEditor::mouseDown(const juce::MouseEvent& e)
{
    // Zone STOP
    int stopX = W - MARGIN - STOP_W;
    int stopY = 12;
    if (e.position.x >= stopX && e.position.x <= stopX + STOP_W &&
        e.position.y >= stopY && e.position.y <= stopY + STOP_H)
    {
        proc.stopEffect();
    }
}

// ─── PAINT ─────────────────────────────────────────────────────
void GranularReverbEditor::paint(juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    g.fillAll(bg);

    g.setColour(line.withAlpha(0.6f));
    g.drawRoundedRectangle(3, 3, w - 6, h - 6, 6.0f, 1.0f);

    // Titre
    g.setColour(textMain);
    g.setFont(makeFontBold(22.0f));
    g.drawText("GRANULAR REVERB", 0, 10, w - STOP_W - MARGIN, 32, juce::Justification::centred);

    // STOP — carré rouge (dessiné dans paint, pas de bouton JUCE)
    {
        int stopX = W - MARGIN - STOP_W;
        int stopY = 12;
        g.setColour(juce::Colour(0xffcc2222));
        g.fillRect(stopX, stopY, STOP_W, STOP_H);
        g.setColour(juce::Colours::white);
        g.drawRect(stopX, stopY, STOP_W, STOP_H, 2);
        g.setFont(makeFontBold(12.0f));
        g.drawText("STOP", stopX, stopY + 5, STOP_W, 16, juce::Justification::centred);
    }

    // Ligne sous titre
    g.setColour(purple.withAlpha(0.6f));
    g.fillRect(MARGIN, HDR_H - 4, w - 2 * MARGIN, 1);

    // Preset bg
    g.setColour(panel);
    g.fillRoundedRectangle(MARGIN - 2, PRESET_Y - 2, W - 2 * MARGIN + 4, PRESET_H + 4, 4.0f);
    g.setColour(line.withAlpha(0.5f));
    g.drawRoundedRectangle(MARGIN - 2, PRESET_Y - 2, W - 2 * MARGIN + 4, PRESET_H + 4, 4.0f, 1.0f);

    // Ligne avant sliders
    g.setColour(line.withAlpha(0.4f));
    g.fillRect(MARGIN, ROW_START - 6, w - 2 * MARGIN, 1);

    // Sliders
    juce::Slider* main[9] = {&sDecay, &sEvolve, &sGrain, &sRate, &sFrz, &sWidth, &sDelay, &sTone, &sPreDelay};

    for (int i = 0; i < 9; ++i)
    {
        int y = ROW_START + i * ROW_H;

        g.setColour(textMain);
        g.setFont(makeFont(14.0f));
        g.drawText(paramNames[i], MARGIN, y + 10, SLIDER_X - MARGIN - 10, 18,
                   juce::Justification::centredLeft);

        g.setColour(track);
        g.fillRoundedRectangle(SLIDER_X, y + 18, SLIDER_W, SLIDER_H, 4.0f);

        float frac = main[i]->getValue() / main[i]->getMaximum();
        if (frac > 0.01f)
        {
            g.setColour((i == 0 || i == 1) ? purple : gold);
            g.fillRoundedRectangle(SLIDER_X, y + 18, (int)(SLIDER_W * frac), SLIDER_H, 4.0f);
        }

        g.setColour((i == 0 || i == 1) ? purple : gold);
        g.setFont(makeFont(13.0f));
        g.drawText(juce::String(main[i]->getValue(), 1), VAL_X, y + 10, VAL_W, 18,
                   juce::Justification::centredRight);
    }

    // Ligne avant asym/dry/wet
    int yAsym = ROW_START + 9 * ROW_H + 4;
    g.setColour(line.withAlpha(0.4f));
    g.fillRect(MARGIN, yAsym - 4, w - 2 * MARGIN, 1);

    // LED Asym
    bool asymOn = proc.apvts.getParameter("asym")->getValue() > 0.5f;
    int ledX = MARGIN + 9;
    int ledY = yAsym + 9;

    if (asymOn)
    {
        g.setColour(purple.withAlpha(0.3f));
        g.fillEllipse(ledX - 9, ledY - 9, 18, 18);
        g.setColour(purple.withAlpha(0.6f));
        g.fillEllipse(ledX - 6, ledY - 6, 12, 12);
        g.setColour(purple);
        g.fillEllipse(ledX - 4, ledY - 4, 8, 8);
    }
    else
    {
        g.setColour(ledOff);
        g.fillEllipse(ledX - 4, ledY - 4, 8, 8);
        g.setColour(line);
        g.drawEllipse(ledX - 4, ledY - 4, 8, 8, 1.0f);
    }

    g.setColour(textMain.withAlpha(0.7f));
    g.setFont(makeFont(13.0f));
    g.drawText("Asym L/R", MARGIN + 24, yAsym + 2, 90, 16, juce::Justification::centredLeft);

    // Dry
    int yDry = yAsym + 30;
    g.setColour(textMain);
    g.setFont(makeFont(14.0f));
    g.drawText("Dry", MARGIN, yDry + 10, 90, 18, juce::Justification::centredLeft);

    g.setColour(track);
    g.fillRoundedRectangle(SLIDER_X, yDry + 18, SLIDER_W, SLIDER_H, 4.0f);
    float fracD = sDry.getValue() / sDry.getMaximum();
    if (fracD > 0.01f)
    {
        g.setColour(purple);
        g.fillRoundedRectangle(SLIDER_X, yDry + 18, (int)(SLIDER_W * fracD), SLIDER_H, 4.0f);
    }
    g.setColour(purple);
    g.setFont(makeFont(13.0f));
    g.drawText(juce::String(sDry.getValue(), 2), VAL_X, yDry + 10, VAL_W, 18,
               juce::Justification::centredRight);

    // Wet
    int yWet = yDry + 38;
    g.setColour(textMain);
    g.setFont(makeFont(14.0f));
    g.drawText("Wet", MARGIN, yWet + 10, 90, 18, juce::Justification::centredLeft);

    g.setColour(track);
    g.fillRoundedRectangle(SLIDER_X, yWet + 18, SLIDER_W, SLIDER_H, 4.0f);
    float fracW = sWet.getValue() / sWet.getMaximum();
    if (fracW > 0.01f)
    {
        g.setColour(purple);
        g.fillRoundedRectangle(SLIDER_X, yWet + 18, (int)(SLIDER_W * fracW), SLIDER_H, 4.0f);
    }
    g.setColour(purple);
    g.setFont(makeFont(13.0f));
    g.drawText(juce::String(sWet.getValue(), 2), VAL_X, yWet + 10, VAL_W, 18,
               juce::Justification::centredRight);
}

// ─── Helpers ───────────────────────────────────────────────────
void GranularReverbEditor::setEnabled(bool on)
{
    sDecay.setEnabled(on);
    sEvolve.setEnabled(on);
    sGrain.setEnabled(on);
    sRate.setEnabled(on);
    sFrz.setEnabled(on);
    sWidth.setEnabled(on);
    sDelay.setEnabled(on);
    sTone.setEnabled(on);
    sPreDelay.setEnabled(on);
    asymBtn.setEnabled(on);
    sDry.setEnabled(on);
    sWet.setEnabled(on);
}

void GranularReverbEditor::applyPreset(int idx)
{
    if (idx < 0 || idx >= 14) return;
    if (idx == 0) { setEnabled(true); return; }
    setEnabled(false);
    proc.apvts.getParameter("decay")->setValueNotifyingHost(presetVals[idx][0]);
    proc.apvts.getParameter("evolve")->setValueNotifyingHost(presetVals[idx][1]);
    proc.apvts.getParameter("grainSize")->setValueNotifyingHost(presetVals[idx][2]);
    proc.apvts.getParameter("rate")->setValueNotifyingHost(presetVals[idx][3]);
    proc.apvts.getParameter("freeze")->setValueNotifyingHost(presetVals[idx][4]);
    proc.apvts.getParameter("width")->setValueNotifyingHost(presetVals[idx][5]);
    proc.apvts.getParameter("delay")->setValueNotifyingHost(presetVals[idx][6]);
    proc.apvts.getParameter("tone")->setValueNotifyingHost(presetVals[idx][7]);
    proc.apvts.getParameter("preDelay")->setValueNotifyingHost(presetVals[idx][8]);
    proc.apvts.getParameter("asym")->setValueNotifyingHost(presetVals[idx][9]);
    proc.apvts.getParameter("dry")->setValueNotifyingHost(presetVals[idx][10]);
    proc.apvts.getParameter("wet")->setValueNotifyingHost(presetVals[idx][11]);
    setEnabled(true);
    repaint();
}
