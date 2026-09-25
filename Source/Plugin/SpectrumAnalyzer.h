#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
// Runs on the message thread. The audio callback only writes to an SPSC FIFO.
class SpectrumAnalyzer {
public:
    static constexpr int points = 1024;
    SpectrumAnalyzer() : transforms{juce::dsp::FFT(10), juce::dsp::FFT(11),
                                   juce::dsp::FFT(12), juce::dsp::FFT(13)} {
        values.fill(-120.0f); configure(48000, 2);
    }
    void configure(double sampleRate, int resolution) {
        rate = sampleRate > 0 ? sampleRate : 48000;
        index = juce::jlimit(0, 3, resolution); n = 1 << (10 + index);
        write = count = sinceFrame = 0; windowSum = 0;
        for (int j=0;j<n;++j) {
            window[j] = .5f - .5f * std::cos(juce::MathConstants<float>::twoPi*j/n);
            windowSum += window[j];
        }
    }
    void push(float left, float right) {
        ring[0][write] = left; ring[1][write] = right; write=(write+1)%n;
        count = std::min(n,count+1);
        if (++sinceFrame < n/8 || count < n) return;
        sinceFrame = 0;
        for (int ch=0;ch<2;++ch) {
            buffer[ch].fill(0);
            for (int j=0;j<n;++j) buffer[ch][j]=ring[ch][(write+j)%n]*window[j];
            transforms[index].performFrequencyOnlyForwardTransform(buffer[ch].data());
        }
        for (int i=0;i<points;++i) {
            const float hz=20.f*std::pow(1000.f,float(i)/(points-1));
            const float bin=juce::jlimit(0.f,float(n/2),float(hz*n/rate));
            const int k=int(bin), next=std::min(n/2,k+1);const float fraction=bin-k;
            const float a=std::max(buffer[0][k],buffer[1][k]);
            const float b=std::max(buffer[0][next],buffer[1][next]);
            values[i]=20.f*std::log10(std::max(1e-8f,(a+(b-a)*fraction)*2.f/windowSum));
        }
    }
    std::array<float,points> values{};
private:
    std::array<juce::dsp::FFT,4> transforms;
    std::array<std::array<float,8192>,2> ring{};
    std::array<std::array<float,16384>,2> buffer{};
    std::array<float,8192> window{};
    int index=2,n=4096,write=0,count=0,sinceFrame=0;
    double rate=48000;float windowSum=1;
};
