#pragma once
#include "GranularReverbProcessor.h"

class GranularReverbEditor : public juce::AudioProcessorEditor
{
public:
    GranularReverbEditor(GranularReverbProcessor&);
    ~GranularReverbEditor() override = default;

private:
    juce::ComboBox presetBox;
    juce::Slider sDecay, sEvolve, sGrain, sRate, sFrz, sWidth, sDelay, sTone;
    juce::Slider sDry, sWet;
    juce::AudioProcessorValueTreeState::SliderAttachment
        aDecay, aEvolve, aGrain, aRate, aFrz, aWidth, aDelay, aTone, aDry, aWet;
    GranularReverbProcessor& proc;

    void applyPreset(int idx);
    void setEnabled(bool on);

    JUCE_DECLARE_NON_COPYABLE(GranularReverbEditor)
};
