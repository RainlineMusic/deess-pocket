#include <JuceHeader.h>
#include "../Source/Plugin/PluginProcessor.h"
#include "../Source/Plugin/PluginEditor.h"
#include <cmath>
#include <memory>
#include <random>

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    juce::ScopedJuceInitialiser_GUI initialise;
    DeessPocketProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    std::mt19937 rng(841);
    std::uniform_real_distribution<float> noise(-1.0f, 1.0f);
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> block(2, 512);
    for (int offset = 0; offset < 24576; offset += block.getNumSamples()) {
        for (int i = 0; i < block.getNumSamples(); ++i) {
            const auto time = float(offset + i) / 48000.0f;
            const float voice = 0.12f * std::sin(juce::MathConstants<float>::twoPi * 215.0f * time)
                              + 0.055f * std::sin(juce::MathConstants<float>::twoPi * 430.0f * time)
                              + 0.20f * noise(rng);
            block.setSample(0, i, voice);
            block.setSample(1, i, voice);
        }
        processor.processBlock(block, midi);
    }
    static_cast<DeessPocketEditor*>(editor.get())->refreshForSnapshot();
    const auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds(), true);
    const juce::File target(argv[1]);
    auto stream = target.createOutputStream();
    if (!stream) return 2;
    juce::PNGImageFormat png;
    return png.writeImageToStream(snapshot, *stream) ? 0 : 3;
}
