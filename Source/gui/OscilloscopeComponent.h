#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace TimeDilationDAW
{

class OscilloscopeComponent : public juce::Component
{
public:
    OscilloscopeComponent();
    ~OscilloscopeComponent() override = default;

    void pushBuffer(const juce::AudioBuffer<float>& buffer);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    static constexpr size_t kHistorySize = 512;
    std::vector<float> waveformHistory;
    size_t writePos = 0;
    float peakLevel = 0.0f;
};

} // namespace TimeDilationDAW
