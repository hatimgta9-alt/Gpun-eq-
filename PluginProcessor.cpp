#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

GPUNEQAudioProcessor::GPUNEQAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    bands.push_back({0, 100.0f, 0.0f, 0.707f});
    bands.push_back({0, 1000.0f, 0.0f, 0.707f});
    bands.push_back({0, 8000.0f, 0.0f, 0.707f});
}

bool GPUNEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void GPUNEQAudioProcessor::prepareToPlay(double sampleRate, int)
{
    const juce::ScopedLock sl(bandLock);
    currentSampleRate = sampleRate;
    rebuildFilters();
}

void GPUNEQAudioProcessor::Biquad::set(const Band& band, double sr)
{
    const double f = juce::jlimit(10.0, sr * 0.45, (double) band.frequency);
    const double qv = juce::jlimit(0.10, 18.0, (double) band.q);
    const double A = std::pow(10.0, (double) band.gain / 40.0);
    const double w = 2.0 * juce::MathConstants<double>::pi * f / sr;
    const double c = std::cos(w), s = std::sin(w);
    const double alpha = s / (2.0 * qv);
    double bb0=1, bb1=0, bb2=0, aa0=1, aa1=0, aa2=0;

    switch (band.type)
    {
        case 1: // low shelf
        {
            const double beta = 2.0 * std::sqrt(A) * alpha;
            bb0 = A*((A+1)-(A-1)*c+beta);
            bb1 = 2*A*((A-1)-(A+1)*c);
            bb2 = A*((A+1)-(A-1)*c-beta);
            aa0 = (A+1)+(A-1)*c+beta;
            aa1 = -2*((A-1)+(A+1)*c);
            aa2 = (A+1)+(A-1)*c-beta;
            break;
        }
        case 2: // high shelf
        {
            const double beta = 2.0 * std::sqrt(A) * alpha;
            bb0 = A*((A+1)+(A-1)*c+beta);
            bb1 = -2*A*((A-1)+(A+1)*c);
            bb2 = A*((A+1)+(A-1)*c-beta);
            aa0 = (A+1)-(A-1)*c+beta;
            aa1 = 2*((A-1)-(A+1)*c);
            aa2 = (A+1)-(A-1)*c-beta;
            break;
        }
        case 3: // high-pass
            bb0=(1+c)/2; bb1=-(1+c); bb2=(1+c)/2;
            aa0=1+alpha; aa1=-2*c; aa2=1-alpha; break;
        case 4: // low-pass
            bb0=(1-c)/2; bb1=1-c; bb2=(1-c)/2;
            aa0=1+alpha; aa1=-2*c; aa2=1-alpha; break;
        default: // peaking bell
            bb0=1+alpha*A; bb1=-2*c; bb2=1-alpha*A;
            aa0=1+alpha/A; aa1=-2*c; aa2=1-alpha/A; break;
    }
    b0=bb0/aa0; b1=bb1/aa0; b2=bb2/aa0;
    a1=aa1/aa0; a2=aa2/aa0;
}

float GPUNEQAudioProcessor::Biquad::process(float x)
{
    const double y = b0*x + z1;
    z1 = b1*x - a1*y + z2;
    z2 = b2*x - a2*y;
    return (float)y;
}

void GPUNEQAudioProcessor::rebuildFilters()
{
    filters.resize(bands.size());
    for (size_t i=0; i<bands.size(); ++i)
    {
        filters[i].resize((size_t) getTotalNumOutputChannels());
        for (auto& f : filters[i]) { f.set(bands[i], currentSampleRate); f.z1=0; f.z2=0; }
    }
}

void GPUNEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const juce::ScopedLock sl(bandLock);
    const int channels = getTotalNumOutputChannels();
    for (int ch=0; ch<channels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i=0; i<buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            for (size_t b=0; b<filters.size(); ++b)
                x = filters[b][(size_t)ch].process(x);
            data[i] = x;
        }
    }
}

int GPUNEQAudioProcessor::getBandCount() const
{
    const juce::ScopedLock sl(bandLock);
    return (int) bands.size();
}
GPUNEQAudioProcessor::Band GPUNEQAudioProcessor::getBand(int index) const
{
    const juce::ScopedLock sl(bandLock);
    if (juce::isPositiveAndBelow(index, (int)bands.size())) return bands[(size_t)index];
    return {};
}
void GPUNEQAudioProcessor::addBand()
{
    const juce::ScopedLock sl(bandLock);
    bands.push_back({0, 1000.0f, 0.0f, 0.707f});
    rebuildFilters();
}
void GPUNEQAudioProcessor::removeBand(int index)
{
    const juce::ScopedLock sl(bandLock);
    if (juce::isPositiveAndBelow(index, (int)bands.size()) && bands.size() > 1)
    {
        bands.erase(bands.begin()+index);
        rebuildFilters();
    }
}
void GPUNEQAudioProcessor::updateBand(int index, const Band& band)
{
    const juce::ScopedLock sl(bandLock);
    if (juce::isPositiveAndBelow(index, (int)bands.size()))
    {
        bands[(size_t)index] = band;
        for (auto& f : filters[(size_t)index]) { f.set(band, currentSampleRate); f.z1=0; f.z2=0; }
    }
}

void GPUNEQAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::ValueTree state("INFINITYEQ");
    {
        const juce::ScopedLock sl(bandLock);
        for (const auto& b : bands)
        {
            juce::ValueTree node("BAND");
            node.setProperty("type", b.type, nullptr);
            node.setProperty("frequency", b.frequency, nullptr);
            node.setProperty("gain", b.gain, nullptr);
            node.setProperty("q", b.q, nullptr);
            state.appendChild(node, nullptr);
        }
    }
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, dest);
}
void GPUNEQAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (!state.hasType("INFINITYEQ")) return;
        const juce::ScopedLock sl(bandLock);
        bands.clear();
        for (int i=0; i<state.getNumChildren(); ++i)
        {
            auto n = state.getChild(i);
            if (!n.hasType("BAND")) continue;
            Band b;
            b.type = (int)n.getProperty("type", 0);
            b.frequency = (float)n.getProperty("frequency", 1000.0);
            b.gain = (float)n.getProperty("gain", 0.0);
            b.q = (float)n.getProperty("q", 0.707);
            bands.push_back(b);
        }
        if (bands.empty()) bands.push_back({});
        rebuildFilters();
    }
}

juce::AudioProcessorEditor* GPUNEQAudioProcessor::createEditor()
{
    return new GPUNEQAudioProcessorEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GPUNEQAudioProcessor();
}
