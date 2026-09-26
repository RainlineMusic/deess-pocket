#include "PluginProcessor.h"
#include "PluginEditor.h"

DeessPocketProcessor::DeessPocketProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "PARAMETERS", layout()) {
    threshold = params.getRawParameterValue("threshold");
    ratio = params.getRawParameterValue("ratio");
    low = params.getRawParameterValue("low");
    sibilanceGain = params.getRawParameterValue("sibilanceGain");
    bypass = params.getRawParameterValue("bypass");
    setLatencySamples(deess::fftSize - 1);
}

juce::AudioProcessorValueTreeState::ParameterLayout DeessPocketProcessor::layout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold", juce::NormalisableRange<float>(-90.f, 0.f, 0.1f), -52.2f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ratio", "Ratio", juce::NormalisableRange<float>(1.f, 21.f, 0.1f), 4.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "low", "Low", juce::NormalisableRange<float>(-12.f, 0.f, 0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sibilanceGain", "Sibilance Gain", juce::NormalisableRange<float>(-12.f, 12.f, 0.1f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    return {p.begin(), p.end()};
}

void DeessPocketProcessor::prepareToPlay(double sampleRate, int) {
    engine.prepare(sampleRate, getMainBusNumInputChannels());
    setLatencySamples(engine.latencySamples());
}

bool DeessPocketProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
        && (l.getMainInputChannelSet() == juce::AudioChannelSet::mono()
            || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}

void DeessPocketProcessor::process(juce::AudioBuffer<float>& b, bool hostBypass) noexcept {
    juce::ScopedNoDenormals noDenormals;
    engine.setControls({threshold->load(), ratio->load(), low->load(), sibilanceGain->load(),
                        hostBypass || bypass->load() > 0.5f});
    hostBypassDisplay.store(hostBypass, std::memory_order_relaxed);
    if (b.getNumChannels() == 0) return;
    int begin1=0,size1=0,begin2=0,size2=0;
    analyzerFifo.prepareToWrite(b.getNumSamples(),begin1,size1,begin2,size2);
    for (int j=0;j<size1+size2;++j) {
        const int slot=j<size1?begin1+j:begin2+j-size1;
        analyzerAudio[slot]={b.getSample(0,j),b.getSample(b.getNumChannels()-1,j)};
    }
    analyzerFifo.finishedWrite(size1+size2);
    auto* l = b.getWritePointer(0);
    auto* r = b.getNumChannels() > 1 ? b.getWritePointer(1) : nullptr;
    for (int i = 0; i < b.getNumSamples(); ++i) {
        float ol, oright;
        engine.process(l[i], r ? r[i] : l[i], ol, oright);
        l[i] = ol;
        if (r) r[i] = oright;
    }
    const auto m = engine.meters();
    confidenceDisplay.store(m.confidence, std::memory_order_relaxed);
    detectorDisplay.store(m.detectorDb, std::memory_order_relaxed);
    for (int i = 0; i < deess::displayBands; ++i) {
        gainDisplay[i].store(m.gainDb[i], std::memory_order_relaxed);
    }
}
void DeessPocketProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { process(b, false); }
void DeessPocketProcessor::processBlockBypassed(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { process(b, true); }

deess::Meters DeessPocketProcessor::display() const noexcept {
    deess::Meters m;
    m.confidence = confidenceDisplay.load(std::memory_order_relaxed);
    m.detectorDb = detectorDisplay.load(std::memory_order_relaxed);
    for (int i = 0; i < deess::displayBands; ++i) {
        m.gainDb[i] = gainDisplay[i].load(std::memory_order_relaxed);
    }
    return m;
}

int DeessPocketProcessor::readAnalyzer(StereoSample* dest, int maximum) noexcept {
    int b1=0,n1=0,b2=0,n2=0;
    analyzerFifo.prepareToRead(maximum,b1,n1,b2,n2);
    for(int j=0;j<n1;++j) dest[j]=analyzerAudio[b1+j];
    for(int j=0;j<n2;++j) dest[n1+j]=analyzerAudio[b2+j];
    analyzerFifo.finishedRead(n1+n2); return n1+n2;
}

juce::AudioProcessorEditor* DeessPocketProcessor::createEditor() { return new DeessPocketEditor(*this); }
juce::AudioProcessorParameter* DeessPocketProcessor::getBypassParameter() const {
    return params.getParameter("bypass");
}
void DeessPocketProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = params.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void DeessPocketProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.hasType(params.state.getType())) params.replaceState(state);
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DeessPocketProcessor(); }
