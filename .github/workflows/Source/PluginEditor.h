#pragma once
#include "PluginProcessor.h"

class BassDistortionProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BassDistortionProcessorEditor(BassDistortionProcessor&);
    ~BassDistortionProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BassDistortionProcessor& processorRef;

    // Filtre
    juce::Slider freqOffsetSlider, qSlider;
    juce::Label  freqOffsetLabel, qLabel;

    // Bandes
    std::array<juce::Slider, 5> bandSliders;
    std::array<juce::Label, 5>  bandLabels;

    // Distorsion
    juce::Slider driveSlider, biasSlider, mixSlider;
    juce::Label  driveLabel, biasLabel, mixLabel;

    // Feedback
    juce::Slider fb1Slider, fb2Slider;
    juce::Label  fb1Label, fb2Label;

    // Sortie
    juce::Slider outSlider;
    juce::Label  outLabel;

    // Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> freqAtt, qAtt;
    std::array<std::unique_ptr<SliderAttachment>, 5> bandAtts;
    std::unique_ptr<SliderAttachment> driveAtt, biasAtt, mixAtt;
    std::unique_ptr<SliderAttachment> fb1Att, fb2Att, outAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassDistortionProcessorEditor)
};
