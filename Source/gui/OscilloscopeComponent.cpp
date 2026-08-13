#include "OscilloscopeComponent.h"
#include "CarbonGoldLookAndFeel.h"
#include <algorithm>

namespace TimeDilationDAW
{

OscilloscopeComponent::OscilloscopeComponent()
{
    waveformHistory.resize(kHistorySize, 0.0f);
}

void OscilloscopeComponent::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() == 0) return;

    const float* readL = buffer.getReadPointer(0);
    int numSamps = buffer.getNumSamples();
    float currentPeak = buffer.getMagnitude(0, numSamps);

    peakLevel = std::max(currentPeak, peakLevel * 0.92f);

    for (int i = 0; i < numSamps; ++i)
    {
        waveformHistory[writePos] = readL[i];
        writePos = (writePos + 1) % kHistorySize;
    }
    repaint();
}

void OscilloscopeComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Dark Panel Background
    g.setColour(juce::Colour::fromRGB(0x0c, 0x0c, 0x12));
    g.fillRoundedRectangle(b, 4.0f);

    g.setColour(juce::Colour::fromRGB(0x22, 0x22, 0x30));
    g.drawRoundedRectangle(b, 4.0f, 1.0f);

    // Center Zero Line
    float midY = b.getCentreY();
    g.setColour(juce::Colour::fromRGB(0x1e, 0x29, 0x3b));
    g.drawHorizontalLine(static_cast<int>(midY), b.getX(), b.getRight());

    // Draw Oscilloscope Path
    juce::Path p;
    float xStep = b.getWidth() / static_cast<float>(kHistorySize);
    float hHalf = (b.getHeight() - 4.0f) / 2.0f;

    for (size_t i = 0; i < kHistorySize; ++i)
    {
        size_t idx = (writePos + i) % kHistorySize;
        float sampleVal = waveformHistory[idx];
        float x = b.getX() + i * xStep;
        float y = midY - sampleVal * hHalf;

        if (i == 0)
            p.startNewSubPath(x, y);
        else
            p.lineTo(x, y);
    }

    // Glow Effect
    g.setColour(CarbonGoldLookAndFeel::cyberCyan.withAlpha(0.25f));
    g.strokePath(p, juce::PathStrokeType(4.0f));

    g.setColour(CarbonGoldLookAndFeel::cyberCyan);
    g.strokePath(p, juce::PathStrokeType(1.5f));

    // Peak Level Meter Bar
    auto meterRect = b.removeFromRight(8.0f).reduced(2.0f);
    g.setColour(juce::Colour::fromRGB(0x1e, 0x29, 0x3b));
    g.fillRect(meterRect);

    float meterH = meterRect.getHeight() * std::clamp(peakLevel, 0.0f, 1.0f);
    auto fillRect = meterRect.removeFromBottom(meterH);

    g.setColour(peakLevel > 0.95f ? juce::Colour::fromRGB(0xef, 0x44, 0x44) : CarbonGoldLookAndFeel::goldAccent);
    g.fillRect(fillRect);
}

void OscilloscopeComponent::resized()
{
}

} // namespace TimeDilationDAW
