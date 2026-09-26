#include "PluginEditor.h"
#include "BinaryData.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float pi = 3.14159265358979323846f;
constexpr int baseWidth = 1536, baseHeight = 922, headerHeight = 104;
const juce::Colour pale(0xffe6f1f6), magenta(0xffff18ed);
float responseY(float db) { return 162.0f - db * (db > 0.0f ? 4.7f : 9.0f); }
}

juce::Font DeessPocketEditor::font(float size) const {
#if JUCE_MAC
    return juce::Font("SF Pro Display", "Light", size);
#elif JUCE_WINDOWS
    return juce::Font("Segoe UI", "Light", size);
#else
    return juce::Font(juce::Font::getDefaultSansSerifFontName(), size, juce::Font::plain);
#endif
}

void DeessPocketEditor::DialStyle::drawRotarySlider(juce::Graphics& g, int x, int y,
    int w, int h, float proportion, float start, float end, juce::Slider& slider) {
    const auto centre = juce::Point<float>(x + w * .5f, y + h * .5f);
    const float radius = std::min(w, h) * .405f;
    const auto colour = slider.getName() == "THRESHOLD" ? pale : magenta;
    juce::Path ring, active;
    ring.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    active.addCentredArc(centre.x, centre.y, radius, radius, 0, start,
                         start + proportion * (end - start), true);
    auto face = juce::ImageCache::getFromMemory(BinaryData::dialface_png,
                                                 BinaryData::dialface_pngSize);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(face, {centre.x - radius, centre.y - radius, radius * 2, radius * 2},
                juce::RectanglePlacement::stretchToFit);
    g.setColour(juce::Colour(0xff50606a).withAlpha(.55f));
    g.strokePath(ring, juce::PathStrokeType(2.5f));
    g.setColour(colour.withAlpha(.055f));
    g.strokePath(active, juce::PathStrokeType(29.0f));
    g.setColour(colour.withAlpha(.12f));
    g.strokePath(active, juce::PathStrokeType(16.0f));
    g.setColour(colour.withAlpha(.31f));
    g.strokePath(active, juce::PathStrokeType(8.0f));
    g.setColour(colour);
    g.strokePath(active, juce::PathStrokeType(3.6f));
    const float angle = start + proportion * (end - start);
    g.setColour(juce::Colours::white);
    g.drawLine(centre.x + std::sin(angle) * radius * .32f,
               centre.y - std::cos(angle) * radius * .32f,
               centre.x + std::sin(angle) * radius * .67f,
               centre.y - std::cos(angle) * radius * .67f, 3.0f);
}

DeessPocketEditor::DeessPocketEditor(DeessPocketProcessor& p)
    : AudioProcessorEditor(p), processor(p) {
    const char* ids[] = {"threshold", "ratio", "low", "sibilanceGain"};
    const char* titles[] = {"THRESHOLD", "RATIO", "LOW", "SIBILANCE GAIN"};
    for (int i = 0; i < 4; ++i) {
        dials[i].setName(titles[i]);
        dials[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        dials[i].setRotaryParameters(1.25f * pi, 2.75f * pi, true);
        dials[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        dials[i].setLookAndFeel(&style);
        addAndMakeVisible(dials[i]);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.params, ids[i], dials[i]);
    }
    menuButton.setButtonText({});
    powerButton.setButtonText({});
    menuButton.setLookAndFeel(&chromeStyle);
    powerButton.setLookAndFeel(&chromeStyle);
    menuButton.onClick = [this] { showMenu(); };
    powerButton.setClickingTogglesState(true);
    powerButton.onClick = [this] {
        previousBypass = powerButton.getToggleState();
        repaint();
    };
    addAndMakeVisible(menuButton);
    addAndMakeVisible(powerButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.params, "bypass", powerButton);
    background = juce::ImageCache::getFromMemory(BinaryData::background_png,
                                                  BinaryData::background_pngSize);
    analyzer.configure(processor.getSampleRate(), detail);
    visualSpectrum.fill(-90.0f);
    setResizable(false, false);
    setSize(baseWidth, baseHeight);
    startTimerHz(30);
}

DeessPocketEditor::~DeessPocketEditor() {
    stopTimer();
    bypassAttachment.reset();
    attachments = {};
    for (auto& d : dials) d.setLookAndFeel(nullptr);
    menuButton.setLookAndFeel(nullptr);
    powerButton.setLookAndFeel(nullptr);
}

void DeessPocketEditor::resized() {
    const float s = getWidth() / float(baseWidth);
    auto rect = [s](float x, float y, float w, float h) {
        return juce::Rectangle<int>(int(x * s), int(y * s), int(w * s), int(h * s));
    };
    const int centres[] = {475, 666, 855, 1048};
    for (int i = 0; i < 4; ++i)
        dials[i].setBounds(rect(float(centres[i] - 70), 698, 140, 135));
    menuButton.setBounds(rect(20, 13, 66, 62));
    powerButton.setBounds(rect(1452, 14, 66, 64));
}

void DeessPocketEditor::timerCallback() {
    display = processor.display();
    for (;;) {
        const int received = processor.readAnalyzer(analyzerInput.data(), int(analyzerInput.size()));
        for (int i = 0; i < received; ++i)
            analyzer.push(analyzerInput[i][0], analyzerInput[i][1]);
        if (received < int(analyzerInput.size())) break;
    }
    const float release = speed == 0 ? .28f : speed == 1 ? .50f : .78f;
    for (int i = 0; i < SpectrumAnalyzer::points; ++i) {
        const float current = analyzer.values[i];
        visualSpectrum[i] = current > visualSpectrum[i] ? current
                            : current + release * (visualSpectrum[i] - current);
    }
    const bool bypassNow = powerButton.getToggleState() || processor.hostBypassed();
    if (!bypassNow && ++snapshotCounter >= 45) {
        snapshotCounter = 0;
        updateBypassSnapshot();
    }
    previousBypass = bypassNow;
    repaint();
}

void DeessPocketEditor::updateBypassSnapshot() {
    const int top = int(headerHeight * getWidth() / float(baseWidth));
    capturingSnapshot = true;
    const auto source = createComponentSnapshot({0, top, getWidth(), getHeight() - top}, true);
    capturingSnapshot = false;
    const int w = std::max(1, source.getWidth() / 3);
    const int h = std::max(1, source.getHeight() / 3);
    juce::Image small(juce::Image::ARGB, w, h, true);
    juce::Graphics sg(small);
    sg.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    sg.drawImage(source, small.getBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    bypassSnapshot = juce::Image(juce::Image::ARGB, w, h, true);
    juce::ImageConvolutionKernel blur(11);
    blur.createGaussianBlur(3.4f);
    blur.applyToImage(bypassSnapshot, small, small.getBounds());
}

void DeessPocketEditor::paint(juce::Graphics& g) {
    const float s = getWidth() / float(baseWidth);
    const auto scale = juce::AffineTransform::scale(s);
    g.fillAll(juce::Colour(0xff000811));
    g.addTransform(scale);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(background, juce::Rectangle<float>(0, 0, baseWidth, baseHeight),
                juce::RectanglePlacement::stretchToFit);
    g.setColour(juce::Colour(0xff163145));
    g.drawLine(0, 103, 1536, 103, 1);

    // Header: lettering and spacing are fixed in reference coordinates.
    g.setColour(juce::Colour(0xff9eb3c2));
    g.setFont(font(35).withExtraKerningFactor(.20f));
    g.drawText("DEESS", 615, 11, 306, 47, juce::Justification::centred);
    g.setFont(font(10).withExtraKerningFactor(.48f));
    g.drawText("POCKET SERIES", 644, 55, 248, 20, juce::Justification::centred);
    g.setColour(juce::Colour(0xff395160));
    g.drawLine(636, 65, 673, 65, .8f); g.drawLine(863, 65, 900, 65, .8f);
    for (int i = 0; i < 3; ++i) g.drawLine(37, 33 + i * 8, 64, 33 + i * 8, 1.2f);
    g.setColour(powerButton.getToggleState() ? juce::Colour(0xffd9eaf1) : juce::Colour(0xff607a8a));
    g.drawEllipse(1465, 26, 41, 41, 1.0f);
    juce::Path power;
    power.addCentredArc(1485.5f, 46.5f, 9, 9, 0, -.84f * pi, .84f * pi, true);
    g.strokePath(power, juce::PathStrokeType(1.8f));
    g.drawLine(1485.5f, 32.5f, 1485.5f, 46.5f, 1.8f);

    // Logarithmic frequency grid and horizontal dB grid.
    const float left = 36.0f, right = 1480.0f;
    const auto freqX = [&](float f) { return left + std::log(f / 20.0f) / std::log(1000.0f) * (right - left); };
    for (float f : {20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f}) {
        const float x = freqX(f);
        g.setColour(juce::Colour(0xff244358).withAlpha(.52f));
        g.drawLine(x, 104, x, 862, .8f);
        g.setColour(juce::Colour(0xffccd3d7));
        g.setFont(font(15));
        const juce::String name = f >= 1000 ? juce::String(int(f / 1000)) + "k" : juce::String(int(f));
        g.drawText(name, int(x - 25), 867, 50, 33, juce::Justification::centred);
    }
    for (int db = 0; db >= -60; db -= 6) {
        const float y = responseY(float(db));
        g.setColour(juce::Colour(0xff24404e).withAlpha(db == 0 ? .82f : .38f));
        g.drawLine(15, y, 1482, y, db == 0 ? 1.f : .65f);
        g.setColour(juce::Colour(0xffabb8c2));
        g.setFont(font(14));
        g.drawText(juce::String(db) + (db == 0 ? " dB" : ""), 1487, int(y - 11), 47, 22,
                   juce::Justification::left);
    }
    for (int db : {6, 12}) {
        const float y = responseY(float(db));
        g.setColour(juce::Colour(0xff9aaebc).withAlpha(.55f));
        g.setFont(font(13));
        g.drawText("+" + juce::String(db), 1487, int(y - 11), 47, 22,
                   juce::Justification::left);
    }

    if (showPre) {
        juce::Path fill;
        for (int i = 0; i < SpectrumAnalyzer::points; ++i) {
            const float f = 20.0f * std::pow(1000.0f, i / float(SpectrumAnalyzer::points - 1));
            const float displayedDb = visualSpectrum[i] + tilt * std::log2(f / 1000.0f);
            const float y = juce::jlimit(145.0f, 862.0f, 812.0f - (displayedDb + range) * (650.0f / range));
            if (i == 0) fill.startNewSubPath(freqX(f), y);
            else fill.lineTo(freqX(f), y);
        }
        auto outline = fill;
        fill.lineTo(right, 862); fill.lineTo(left, 862); fill.closeSubPath();
        juce::ColourGradient spectral(juce::Colour(0xfff8fcff).withAlpha(.94f), 0, 180,
                                      juce::Colour(0xff071a27).withAlpha(.06f), 0, 862, false);
        spectral.addColour(.32, juce::Colour(0xffd6e1e9).withAlpha(.77f));
        spectral.addColour(.70, juce::Colour(0xff587283).withAlpha(.29f));
        g.setGradientFill(spectral);
        g.fillPath(fill);
        g.setColour(juce::Colour(0xffdceaf3).withAlpha(.075f));
        g.strokePath(outline, juce::PathStrokeType(13.0f));
        g.setColour(juce::Colour(0xffedf6fc).withAlpha(.22f));
        g.strokePath(outline, juce::PathStrokeType(5.0f));
        g.setColour(juce::Colour(0xfff4fbff).withAlpha(.88f));
        g.strokePath(outline, juce::PathStrokeType(1.1f));
    }

    // Repair is an absolute horizontal bin threshold on the input spectrum.
    const float threshold = float(dials[0].getValue());
    const float thresholdY = juce::jlimit(145.0f, 862.0f,
        812.0f - (threshold + range) * (650.0f / range));
    const float repairStart = freqX(3000.0f);
    g.setColour(magenta.withAlpha(.055f));
    g.drawLine(repairStart, thresholdY, right, thresholdY, 12.0f);
    g.setColour(magenta.withAlpha(.18f));
    g.drawLine(repairStart, thresholdY, right, thresholdY, 4.0f);
    g.setColour(magenta.withAlpha(.60f));
    g.drawLine(repairStart, thresholdY, right, thresholdY, .8f);

    // One live signed response: Repair plus dynamic Low and Sibilance Gain.
    juce::Path response, responseFill;
    for (int i = 0; i < deess::displayBands; ++i) {
        const float f = 20.0f * std::pow(1000.0f, i / float(deess::displayBands - 1));
        const float y = responseY(display.gainDb[i]);
        if (i == 0) response.startNewSubPath(freqX(f), y);
        else response.lineTo(freqX(f), y);
    }
    responseFill = response;
    responseFill.lineTo(right, responseY(0));
    responseFill.lineTo(left, responseY(0));
    responseFill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(magenta.withAlpha(.20f), 0, 180,
                                           magenta.withAlpha(.005f), 0, 850, false));
    g.fillPath(responseFill);
    const bool active = std::any_of(display.gainDb.begin(), display.gainDb.end(),
                                    [](float db) { return std::abs(db) > .5f; });
    if (active) {
        juce::Path aura = response;
        aura.lineTo(right, 500.0f);
        aura.lineTo(left, 500.0f);
        aura.closeSubPath();
        g.setGradientFill(juce::ColourGradient(magenta.withAlpha(.09f), 0, 195,
                                               magenta.withAlpha(0.0f), 0, 500, false));
        g.fillPath(aura);
    }
    g.setColour(magenta.withAlpha(.065f));
    g.strokePath(response, juce::PathStrokeType(32.0f));
    g.setColour(magenta.withAlpha(.13f));
    g.strokePath(response, juce::PathStrokeType(15.0f));
    g.setColour(magenta.withAlpha(.30f));
    g.strokePath(response, juce::PathStrokeType(6.0f));
    g.setColour(magenta);
    g.strokePath(response, juce::PathStrokeType(1.8f));

    const char* names[] = {"THRESHOLD", "RATIO", "LOW", "SIBILANCE GAIN"};
    const int cx[] = {475, 666, 855, 1048};
    for (int i = 0; i < 4; ++i) {
        const auto colour = i == 0 ? pale : magenta;
        g.setGradientFill(juce::ColourGradient(colour.withAlpha(.13f), float(cx[i]), 767.0f,
                                               colour.withAlpha(0.0f), float(cx[i] + 115),
                                               767.0f, true));
        g.fillEllipse(float(cx[i] - 115), 652.0f, 230.0f, 230.0f);
    }
    g.setColour(pale);
    for (int i = 0; i < 4; ++i) {
        g.setFont(font(17));
        g.drawText(names[i], cx[i] - 90, 674, 180, 31, juce::Justification::centred);
        g.setFont(font(21));
        const auto v = dials[i].getValue();
        const juce::String value = i == 0 ? juce::String(v, 1) + " dB"
            : i == 1 ? (v >= 20.5 ? juce::String::fromUTF8("∞:1")
                                : juce::String(v, 1) + ":1")
            : (v > 0 ? "+" : "") + juce::String(v, 1) + " dB";
        g.drawText(value, cx[i] - 90, 826, 180, 35, juce::Justification::centred);
    }
}

void DeessPocketEditor::paintOverChildren(juce::Graphics& g) {
    if (capturingSnapshot) return;
    if (!(powerButton.getToggleState() || processor.hostBypassed())) return;
    const int top = int(headerHeight * getWidth() / float(baseWidth));
    const auto dest = juce::Rectangle<float>(0, float(top), float(getWidth()), float(getHeight() - top));
    if (bypassSnapshot.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(bypassSnapshot, dest, juce::RectanglePlacement::stretchToFit);
    }
    g.setColour(juce::Colour(0xff00101a).withAlpha(.48f));
    g.fillRect(dest);
    const float s = getWidth() / float(baseWidth);
    g.setColour(juce::Colour(0xffd4e0e6).withAlpha(.82f));
    g.setFont(font(66.0f * s).withExtraKerningFactor(.18f));
    g.drawText("BYPASS", int(450 * s), int(425 * s), int(636 * s), int(100 * s),
               juce::Justification::centred);
}

void DeessPocketEditor::showMenu() {
    juce::PopupMenu menu;
    juce::PopupMenu zoomMenu, analyzer;
    int z = 1000;
    for (int value : {100, 110, 125, 150, 200})
        zoomMenu.addItem(z++, juce::String(value) + "%", true, zoom == value);
    analyzer.addSectionHeader("Spectrum analyzer");
    analyzer.addItem(2001, "Pre spectrum", true, showPre);
    analyzer.addSeparator();
    analyzer.addSectionHeader("Speed");
    analyzer.addItem(2010, "Fast", true, speed == 0);
    analyzer.addItem(2011, "Medium", true, speed == 1);
    analyzer.addItem(2012, "Slow", true, speed == 2);
    analyzer.addSectionHeader("Resolution");
    analyzer.addItem(2020, "Low", true, detail == 0);
    analyzer.addItem(2021, "Medium", true, detail == 1);
    analyzer.addItem(2022, "High", true, detail == 2);
    analyzer.addItem(2023, "Maximum", true, detail == 3);
    analyzer.addSectionHeader("Range");
    analyzer.addItem(2030, "60 dB", true, range == 60);
    analyzer.addItem(2031, "90 dB", true, range == 90);
    analyzer.addItem(2032, "120 dB", true, range == 120);
    analyzer.addSectionHeader("Tilt");
    analyzer.addItem(2040, "0 dB/oct", true, tilt == 0);
    analyzer.addItem(2041, "3 dB/oct", true, tilt == 3);
    analyzer.addItem(2042, "4.5 dB/oct", true, tilt == 4.5f);
    menu.addSubMenu("Scale", zoomMenu);
    menu.addSubMenu("Spectrum", analyzer);
    auto safe = juce::Component::SafePointer<DeessPocketEditor>(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(menuButton),
                       [safe](int choice) {
        if (safe == nullptr) return;
        if (choice >= 1000 && choice <= 1004) {
            const int values[] = {100, 110, 125, 150, 200};
            safe->zoom = values[choice - 1000];
            safe->setSize(baseWidth * safe->zoom / 100, baseHeight * safe->zoom / 100);
        } else if (choice == 2001) safe->showPre = !safe->showPre;
        else if (choice >= 2010 && choice <= 2012) safe->speed = choice - 2010;
        else if (choice >= 2020 && choice <= 2023) {
            safe->detail = choice - 2020;
            safe->analyzer.configure(safe->processor.getSampleRate(), safe->detail);
        }
        else if (choice >= 2030 && choice <= 2032) safe->range = 60 + (choice - 2030) * 30;
        else if (choice >= 2040 && choice <= 2042) safe->tilt = choice == 2040 ? 0.f : choice == 2041 ? 3.f : 4.5f;
        safe->repaint();
    });
}
