#include "PluginEditor.h"

BassDistortionProcessorEditor::BassDistortionProcessorEditor(BassDistortionProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // --- Configuration des sliders ---
    auto setupRotary = [](juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(11.0f));
    };

    // Filtre
    setupRotary(freqOffsetSlider, freqOffsetLabel, "Freq (st)");
    setupRotary(qSlider, qLabel, "Q");
    addAndMakeVisible(freqOffsetSlider);
    addAndMakeVisible(freqOffsetLabel);
    addAndMakeVisible(qSlider);
    addAndMakeVisible(qLabel);

    // Bandes
    for (int i = 0; i < 5; ++i)
    {
        setupRotary(bandSliders[i], bandLabels[i],
                    juce::String(BassDistortionProcessor::BASE_FREQS[i], 0) + " Hz");
        addAndMakeVisible(bandSliders[i]);
        addAndMakeVisible(bandLabels[i]);
    }

    // Distorsion
    setupRotary(driveSlider, driveLabel, "Drive");
    setupRotary(biasSlider, biasLabel, "Bias");
    setupRotary(mixSlider, mixLabel, "Mix");
    addAndMakeVisible(driveSlider);
    addAndMakeVisible(driveLabel);
    addAndMakeVisible(biasSlider);
    addAndMakeVisible(biasLabel);
    addAndMakeVisible(mixSlider);
    addAndMakeVisible(mixLabel);

    // Feedback
    setupRotary(fb1Slider, fb1Label, "FB 1");
    setupRotary(fb2Slider, fb2Label, "FB 2");
    addAndMakeVisible(fb1Slider);
    addAndMakeVisible(fb1Label);
    addAndMakeVisible(fb2Slider);
    addAndMakeVisible(fb2Label);

    // Sortie
    setupRotary(outSlider, outLabel, "Out (dB)");
    addAndMakeVisible(outSlider);
    addAndMakeVisible(outLabel);

    // --- Attachments ---
    freqAtt  = std::make_unique<SliderAttachment>(processorRef.apvts, "freqOffset", freqOffsetSlider);
    qAtt     = std::make_unique<SliderAttachment>(processorRef.apvts, "qFactor", qSlider);

    for (int i = 0; i < 5; ++i)
        bandAtts[i] = std::make_unique<SliderAttachment>(
            processorRef.apvts, "band" + juce::String(i) + "Gain", bandSliders[i]);

    driveAtt = std::make_unique<SliderAttachment>(processorRef.apvts, "drive", driveSlider);
    biasAtt  = std::make_unique<SliderAttachment>(processorRef.apvts, "bias", biasSlider);
    mixAtt   = std::make_unique<SliderAttachment>(processorRef.apvts, "mix", mixSlider);

    fb1Att   = std::make_unique<SliderAttachment>(processorRef.apvts, "fb1", fb1Slider);
    fb2Att   = std::make_unique<SliderAttachment>(processorRef.apvts, "fb2", fb2Slider);

    outAtt   = std::make_unique<SliderAttachment>(processorRef.apvts, "output", outSlider);

    setSize(900, 420);
    setResizable(true, true);
}

BassDistortionProcessorEditor::~BassDistortionProcessorEditor() {}

void BassDistortionProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.3f));
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("BASS DISTORTION", getLocalBounds().removeFromTop(30),
               juce::Justification::centred);
}

void BassDistortionProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(30); // titre

    const int sliderSize = 80;
    const int labelH = 18;
    const int cellW = 90;
    const int cellH = sliderSize + labelH + 10;

    // --- Ligne 1 : Filtre ---
    auto row1 = area.removeFromTop(cellH);
    freqOffsetSlider.setBounds(row1.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    freqOffsetLabel.setBounds(freqOffsetSlider.getBounds().translated(0, sliderSize).withHeight(labelH));

    qSlider.setBounds(row1.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    qLabel.setBounds(qSlider.getBounds().translated(0, sliderSize).withHeight(labelH));

    // --- Ligne 2 : Bandes ---
    auto row2 = area.removeFromTop(cellH);
    for (int i = 0; i < 5; ++i)
    {
        auto cell = row2.removeFromLeft(cellW);
        bandSliders[i].setBounds(cell.withSizeKeepingCentre(sliderSize, sliderSize));
        bandLabels[i].setBounds(bandSliders[i].getBounds().translated(0, sliderSize).withHeight(labelH));
    }

    // --- Ligne 3 : Distorsion + Feedback ---
    auto row3 = area.removeFromTop(cellH);
    driveSlider.setBounds(row3.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    driveLabel.setBounds(driveSlider.getBounds().translated(0, sliderSize).withHeight(labelH));

    biasSlider.setBounds(row3.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    biasLabel.setBounds(biasSlider.getBounds().translated(0, sliderSize).withHeight(labelH));

    mixSlider.setBounds(row3.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    mixLabel.setBounds(mixSlider.getBounds().translated(0, sliderSize).withHeight(labelH));

    fb1Slider.setBounds(row3.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    fb1Label.setBounds(fb1Slider.getBounds().translated(0, sliderSize).withHeight(labelH));

    fb2Slider.setBounds(row3.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    fb2Label.setBounds(fb2Slider.getBounds().translated(0, sliderSize).withHeight(labelH));

    outSlider.setBounds(row3.removeFromLeft(cellW).withSizeKeepingCentre(sliderSize, sliderSize));
    outLabel.setBounds(outSlider.getBounds().translated(0, sliderSize).withHeight(labelH));
}
