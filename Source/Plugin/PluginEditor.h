#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"
#include <array>

class DeessPocketEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit DeessPocketEditor(DeessPocketProcessor&);
    ~DeessPocketEditor() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void refreshForSnapshot() { timerCallback(); }

private:
    struct DialStyle final : juce::LookAndFeel_V4 {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float,
                              juce::Slider&) override;
    } style;
    struct ChromeStyle final : juce::LookAndFeel_V4 {
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                                  bool, bool) override {}
        void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override {}
    } chromeStyle;
    void timerCallback() override;
    void showMenu();
    void updateBypassSnapshot();
    juce::Font font(float size) const;
    DeessPocketProcessor& processor;
    std::array<juce::Slider, 4> dials;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attachments;
    juce::TextButton menuButton, powerButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    deess::Meters display{};
    SpectrumAnalyzer analyzer;
    std::array<DeessPocketProcessor::StereoSample, 4096> analyzerInput{};
    juce::Image background;
    juce::Image bypassSnapshot;
    bool previousBypass = false;
    bool capturingSnapshot = false;
    int snapshotCounter = 44;
    int zoom = 100, speed = 0, detail = 2, range = 90;
    float tilt = 3.0f;
    bool showPre = true;
    std::array<float, SpectrumAnalyzer::points> visualSpectrum{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeessPocketEditor)
};
