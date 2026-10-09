#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class BassDistortionProcessor : public juce::AudioProcessor
{
public:
    BassDistortionProcessor();
    ~BassDistortionProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Bass Distortion"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Filtres par bande (stéréo)
    struct BandFilter
    {
        juce::dsp::IIR::Filter<float> left, right;
        float baseFreq;
    };
    std::array<BandFilter, 5> bandFilters;

    // Oversampling 4x pour éviter l'aliasing
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Buffers pour les boucles de feedback
    juce::AudioBuffer<float> feedbackBuffer1, feedbackBuffer2;
    int feedbackWritePos1 = 0, feedbackWritePos2 = 0;

    void updateFilterCoefficients(double sampleRate);
    void processBand(juce::AudioBuffer<float>& buffer, int bandIndex, float gainDb);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassDistortionProcessor)
};
