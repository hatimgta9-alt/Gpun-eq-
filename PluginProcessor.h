#pragma once
#include <JuceHeader.h>
#include <vector>

class GPUNEQAudioProcessor : public juce::AudioProcessor
{
public:
    struct Band
    {
        int type = 0; // 0 Bell, 1 Low Shelf, 2 High Shelf, 3 High Pass, 4 Low Pass
        float frequency = 1000.0f;
        float gain = 0.0f;
        float q = 0.707f;
    };

    GPUNEQAudioProcessor();
    ~GPUNEQAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "GPUN EQ"; }
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

    int getBandCount() const;
    Band getBand(int index) const;
    void addBand();
    void removeBand(int index);
    void updateBand(int index, const Band& band);
    juce::CriticalSection& getBandLock() { return bandLock; }

private:
    struct Biquad
    {
        double b0=1, b1=0, b2=0, a1=0, a2=0;
        double z1=0, z2=0;
        void set(const Band&, double sr);
        float process(float x);
    };

    mutable juce::CriticalSection bandLock;
    std::vector<Band> bands;
    std::vector<std::vector<Biquad>> filters; // band x channel
    double currentSampleRate = 44100.0;
    void rebuildFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GPUNEQAudioProcessor)
};
