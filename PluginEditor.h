#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class GPUNEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit GPUNEQAudioProcessorEditor(GPUNEQAudioProcessor&);
    ~GPUNEQAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void rebuildBandControls();
    void updateSelectedBand();
    void applyControls();
    void drawResponse(juce::Graphics&, juce::Rectangle<float>);

    GPUNEQAudioProcessor& audioProcessor;
    bool compact = false;
    int selectedBand = 0;
    juce::TextButton sizeButton {"COMPACT"};
    juce::TextButton addButton {"+ ADD BAND"};
    juce::TextButton removeButton {"REMOVE"};
    juce::ComboBox bandSelector, typeSelector;
    juce::Slider frequency, gain, q;
    juce::Label freqLabel, gainLabel, qLabel, titleLabel;
    juce::String statusText;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GPUNEQAudioProcessorEditor)
};
