#include "PluginProcessor.h"
#include "PluginEditor.h"

// Fréquences de base (espacées d'une quinte juste)
static constexpr float BASE_FREQS[5] = { 40.0f, 60.0f, 90.0f, 135.0f, 202.0f };

BassDistortionProcessor::BassDistortionProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
}

BassDistortionProcessor::~BassDistortionProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
BassDistortionProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Filtre paramétrique (réglages communs) ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "freqOffset", "Fréquence (demi-tons)",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "qFactor", "Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));

    // --- Gains par bande (5 bandes) ---
    for (int i = 0; i < 5; ++i)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "band" + juce::String(i) + "Gain",
            "Bande " + juce::String(i + 1) + " (dB)",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    }

    // --- Distorsion ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange<float>(1.0f, 50.0f, 0.01f, 0.3f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "bias", "Biais",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    // --- Boucle de feedback 1 (post-saturation → pré-saturation) ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fb1", "Feedback 1",
        juce::NormalisableRange<float>(0.0f, 0.9f, 0.01f), 0.0f));

    // --- Boucle de feedback 2 (autour du filtre) ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "fb2", "Feedback 2",
        juce::NormalisableRange<float>(0.0f, 0.9f, 0.01f), 0.0f));

    // --- Sortie ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output", "Sortie (dB)",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    return layout;
}

void BassDistortionProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialisation de l'oversampler (4x, FIR pour une qualité maximale)
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        getTotalNumOutputChannels(), 2, // 2 étages = 4x
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true, true);
    oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampler->reset();

    // Initialisation des buffers de feedback
    feedbackBuffer1.setSize(2, samplesPerBlock);
    feedbackBuffer2.setSize(2, samplesPerBlock);
    feedbackBuffer1.clear();
    feedbackBuffer2.clear();
    feedbackWritePos1 = 0;
    feedbackWritePos2 = 0;

    updateFilterCoefficients(sampleRate);
}

void BassDistortionProcessor::releaseResources()
{
    oversampler.reset();
    feedbackBuffer1.setSize(0, 0);
    feedbackBuffer2.setSize(0, 0);
}

void BassDistortionProcessor::updateFilterCoefficients(double sampleRate)
{
    const float q = apvts.getRawParameterValue("qFactor")->load();
    const float offsetSemitones = apvts.getRawParameterValue("freqOffset")->load();

    for (int i = 0; i < 5; ++i)
    {
        // Fréquence décalée musicalement (demi-tons)
        float freq = BASE_FREQS[i] * std::pow(2.0f, offsetSemitones / 12.0f);
        freq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), freq);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, freq, q, 1.0f); // gain appliqué séparément

        bandFilters[i].left.coefficients  = coeffs;
        bandFilters[i].right.coefficients = coeffs;
    }
}

void BassDistortionProcessor::processBand(juce::AudioBuffer<float>& buffer,
                                           int bandIndex, float gainDb)
{
    if (std::abs(gainDb) < 0.01f) return;

    const float linearGain = juce::Decibels::decibelsToGain(gainDb);
    const float q = apvts.getRawParameterValue("qFactor")->load();
    const float offsetSemitones = apvts.getRawParameterValue("freqOffset")->load();

    float freq = BASE_FREQS[bandIndex] * std::pow(2.0f, offsetSemitones / 12.0f);
    freq = juce::jlimit(20.0f, static_cast<float>(getSampleRate() * 0.45), freq);

    // Coefficients avec le gain de la bande
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(), freq, q, linearGain);

    bandFilters[bandIndex].left.coefficients  = coeffs;
    bandFilters[bandIndex].right.coefficients = coeffs;

    // Traitement
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock  = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    bandFilters[bandIndex].left.process(juce::dsp::ProcessContextReplacing<float>(leftBlock));
    bandFilters[bandIndex].right.process(juce::dsp::ProcessContextReplacing<float>(rightBlock));
}

void BassDistortionProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // --- Lecture des paramètres ---
    const float drive   = apvts.getRawParameterValue("drive")->load();
    const float bias    = apvts.getRawParameterValue("bias")->load();
    const float mix     = apvts.getRawParameterValue("mix")->load();
    const float fb1     = apvts.getRawParameterValue("fb1")->load();
    const float fb2     = apvts.getRawParameterValue("fb2")->load();
    const float outGain = juce::Decibels::decibelsToGain(
                              apvts.getRawParameterValue("output")->load());

    // Mise à jour des coefficients de filtre
    updateFilterCoefficients(getSampleRate());

    // ============================================================
    // 1. BOUCLE DE FEEDBACK 2 : autour du filtre
    // ============================================================
    if (fb2 > 0.001f)
    {
        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < numSamples; ++s)
            {
                // Ajout du feedback au signal d'entrée
                data[s] += feedbackBuffer2.getSample(ch, feedbackWritePos2) * fb2;
                feedbackWritePos2 = (feedbackWritePos2 + 1) % feedbackBuffer2.getNumSamples();
            }
        }
    }

    // ============================================================
    // 2. FILTRE PARAMÉTRIQUE (1 à 5 bandes)
    // ============================================================
    for (int i = 0; i < 5; ++i)
    {
        float bandGain = apvts.getRawParameterValue("band" + juce::String(i) + "Gain")->load();
        processBand(buffer, i, bandGain);
    }

    // ============================================================
    // 3. OVERSAMPLING → SATURATION → DOWNSAMPLING
    // ============================================================
    juce::dsp::AudioBlock<float> block(buffer);
    auto oversampledBlock = oversampler->processSamplesUp(block);

    for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(ch);
        for (size_t s = 0; s < oversampledBlock.getNumSamples(); ++s)
        {
            // Saturation asymétrique biaisée
            float x = data[s];
            float driven = x * drive;
            float biased = driven + bias;
            float sat = std::tanh(biased) - std::tanh(bias);
            data[s] = (1.0f - mix) * x + mix * sat;
        }
    }

    oversampler->processSamplesDown(block);

    // ============================================================
    // 4. BOUCLE DE FEEDBACK 1 : post-saturation → pré-saturation
    // ============================================================
    if (fb1 > 0.001f)
    {
        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < numSamples; ++s)
            {
                // Stockage du signal saturé pour la prochaine itération
                feedbackBuffer1.setSample(ch, feedbackWritePos1, data[s]);
                feedbackWritePos1 = (feedbackWritePos1 + 1) % feedbackBuffer1.getNumSamples();

                // Réinjection dans le signal de sortie
                float fbSample = feedbackBuffer1.getSample(ch,
                    (feedbackWritePos1 - 1 + feedbackBuffer1.getNumSamples())
                    % feedbackBuffer1.getNumSamples());
                data[s] += fbSample * fb1;
            }
        }
    }

    // ============================================================
    // 5. GAIN DE SORTIE
    // ============================================================
    buffer.applyGain(outGain);

    // Nettoyage des buffers de feedback pour éviter les fuites
    if (fb2 > 0.001f)
    {
        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        {
            // Le buffer de feedback 2 est rempli avec le signal filtré
            auto* data = buffer.getReadPointer(ch);
            for (int s = 0; s < numSamples; ++s)
                feedbackBuffer2.setSample(ch, s, data[s]);
        }
    }
}

juce::AudioProcessorEditor* BassDistortionProcessor::createEditor()
{
    return new BassDistortionProcessorEditor(*this);
}

void BassDistortionProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BassDistortionProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassDistortionProcessor();
}
