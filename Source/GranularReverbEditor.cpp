#include "GranularReverbEditor.h"

static const char* presetNames[] = {
    "Custom", "Tight", "Room", "Hall", "Frozen", "Drums", "Cloud", "Dark"
};
static const float presetVals[][10] = {
    {0,0,0,0,0,0,0,0,0,0},
    { 0.2f, 0.1f,  60.0f, 14.0f, 0.0f, 0.2f, 0.7f, 0.2f, 0.8f, 0.4f},
    { 0.8f, 0.3f, 150.0f,  6.0f, 0.0f, 0.5f, 1.0f, 0.4f, 0.6f, 0.6f},
    { 2.0f, 0.5f, 250.0f,  3.0f, 0.0f, 0.8f, 1.5f, 0.6f, 0.4f, 0.8f},
    { 2.5f, 0.2f, 350.0f,  2.0f, 1.0f, 0.7f, 1.2f, 0.5f, 0.3f, 0.9f},
    { 0.1f, 0.6f,  40.0f, 18.0f, 0.0f, 0.1f, 0.5f, 0.1f, 0.9f, 0.3f},
    { 3.0f, 0.9f, 450.0f,  1.0f, 0.0f, 0.9f, 2.5f, 0.7f, 0.2f, 0.9f},
    { 1.5f, 0.4f, 200.0f,  4.0f, 0.0f, 0.6f, 2.0f, 0.9f, 0.4f, 0.8f}
};

static const char* paramNames[8] = {
    "Decay", "Evolve", "Grain Size", "Rate", "Freeze", "Width", "Delay", "Tone"
};

static juce::FontOptions makeFont(float size)
{
    return juce::FontOptions("Inter", size, juce::Font::plain);
}

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
      aDry(p.apvts, "dry", sDry),
      aWet(p.apvts, "wet", sWet),
      proc(p)
{
    for (int i = 0; i < 8; ++i)
        presetBox.addItem(presetNames[i], i + 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.setBounds(10, 10, 200, 24);
    presetBox.onChange = [this] { applyPreset(presetBox.getSelectedId() - 1); };
    addAndMakeVisible(presetBox);

    juce::Slider* main[8] = {&sDecay, &sEvolve, &sGrain, &sRate, &sFrz, &sWidth, &sDelay, &sTone};
    const int startY = 46;
    const int rowH = 38;

    for (int i = 0; i < 8; ++i)
    {
        main[i]->setSliderStyle(juce::Slider::LinearHorizontal);
        main[i]->setBounds(10, startY + i * rowH + 16, 130, 20);
        main[i]->setTooltip(paramNames[i]);
        main[i]->onValueChange = [this] { repaint(); };
        addAndMakeVisible(main[i]);
    }

    int yDW = startY + 8 * rowH + 6;

    sDry.setSliderStyle(juce::Slider::LinearHorizontal);
    sDry.setBounds(10, yDW + 16, 95, 20);
    sDry.setTooltip("Dry");
    sDry.onValueChange = [this] { repaint(); };
    addAndMakeVisible(sDry);

    sWet.setSliderStyle(juce::Slider::LinearHorizontal);
    sWet.setBounds(115, yDW + 16, 95, 20);
    sWet.setTooltip("Wet");
    sWet.onValueChange = [this] { repaint(); };
    addAndMakeVisible(sWet);

    setSize(230, yDW + 58);
}

void GranularReverbEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);

    const int startY = 46;
    const int rowH = 38;

    auto fontName = makeFont(14.0f);
    auto fontVal  = makeFont(13.0f);

    juce::Slider* main[8] = {&sDecay, &sEvolve, &sGrain, &sRate, &sFrz, &sWidth, &sDelay, &sTone};

    for (int i = 0; i < 8; ++i)
    {
        int y = startY + i * rowH;
        g.setFont(fontName);
        g.drawText(paramNames[i], 10, y, 100, 14, juce::Justification::centredLeft);
        g.setFont(fontVal);
        g.drawText(juce::String(main[i]->getValue(), 2), 145, y + 16, 60, 16, juce::Justification::centredRight);
    }

    int yDW = startY + 8 * rowH + 6;

    g.setFont(fontName);
    g.drawText("Dry", 10, yDW, 40, 14, juce::Justification::centredLeft);
    g.drawText("Wet", 115, yDW, 40, 14, juce::Justification::centredLeft);

    g.setFont(fontVal);
    g.drawText(juce::String(sDry.getValue(), 2), 10, yDW + 38, 95, 14, juce::Justification::centred);
    g.drawText(juce::String(sWet.getValue(), 2), 115, yDW + 38, 95, 14, juce::Justification::centred);
}

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
}

void GranularReverbEditor::applyPreset(int idx)
{
    if (idx < 0 || idx >= 8) return;
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
    proc.apvts.getParameter("dry")->setValueNotifyingHost(presetVals[idx][8]);
    proc.apvts.getParameter("wet")->setValueNotifyingHost(presetVals[idx][9]);
    setEnabled(true);
    repaint();
}
