#pragma once
#include <JuceHeader.h>
#include "../DSP/DeessEngine.h"
#include <array>
#include <atomic>

class DeessPocketProcessor final : public juce::AudioProcessor {
public:
    DeessPocketProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override { engine.reset(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Deess Pocket"; }
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
    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorValueTreeState params;
    deess::Meters display() const noexcept;
    using StereoSample = std::array<float,2>;
    int readAnalyzer(StereoSample* destination, int maximum) noexcept;
    bool hostBypassed() const noexcept { return hostBypassDisplay.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void process(juce::AudioBuffer<float>&, bool hostBypass) noexcept;
    deess::Engine engine;
    std::atomic<float>* threshold = nullptr;
    std::atomic<float>* wide = nullptr;
    std::atomic<float>* split = nullptr;
    std::atomic<float>* repair = nullptr;
    std::atomic<float>* bypass = nullptr;
    // Meter fields are atomic. Analyzer audio uses a separate SPSC FIFO.
    std::atomic<float> confidenceDisplay{0}, detectorDisplay{-120};
    std::atomic<float> wideDisplay{0}, splitDisplay{0};
    std::array<std::atomic<float>, deess::displayBands> repairDisplay{};
    std::atomic<bool> hostBypassDisplay{false};
    juce::AbstractFifo analyzerFifo{32768};
    std::array<StereoSample,32768> analyzerAudio{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeessPocketProcessor)
};
