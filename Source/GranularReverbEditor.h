#pragma once
#include "GranularReverbProcessor.h"

class GranularReverbEditor : public juce::AudioProcessorEditor
{
public:
    GranularReverbEditor(GranularReverbProcessor&);
    ~GranularReverbEditor() override = default;

private:
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

    static const juce::Colour bg;
    static const juce::Colour panel;
    static const juce::Colour textMain;
    static const juce::Colour gold;
    static const juce::Colour purple;
    static const juce::Colour track;
    static const juce::Colour line;
    static const juce::Colour ledOn;
    static const juce::Colour ledOff;

    juce::ComboBox presetBox;
    juce::Slider sDecay, sEvolve, sGrain, sRate, sFrz, sWidth, sDelay, sTone;
    juce::Slider sPreDelay;
    juce::Slider sDry, sWet;
    juce::ToggleButton asymBtn;

    juce::AudioProcessorValueTreeState::SliderAttachment
        aDecay, aEvolve, aGrain, aRate, aFrz, aWidth, aDelay, aTone,
        aPreDelay, aDry, aWet;
    juce::AudioProcessorValueTreeState::ButtonAttachment aAsym;

    GranularReverbProcessor& proc;

    void applyPreset(int idx);
    void setEnabled(bool on);

    JUCE_DECLARE_NON_COPYABLE(GranularReverbEditor)
};
