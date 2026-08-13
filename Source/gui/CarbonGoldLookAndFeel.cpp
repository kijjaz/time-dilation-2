#include "CarbonGoldLookAndFeel.h"

namespace TimeDilationDAW
{

const juce::Colour CarbonGoldLookAndFeel::carbonBg     = juce::Colour::fromRGB(0x11, 0x11, 0x16);
const juce::Colour CarbonGoldLookAndFeel::slatePanel   = juce::Colour::fromRGB(0x18, 0x18, 0x20);
const juce::Colour CarbonGoldLookAndFeel::cyberCyan   = juce::Colour::fromRGB(0x06, 0xb6, 0xd4);
const juce::Colour CarbonGoldLookAndFeel::royalViolet  = juce::Colour::fromRGB(0x8b, 0x5c, 0xf6);
const juce::Colour CarbonGoldLookAndFeel::goldAccent   = juce::Colour::fromRGB(0xea, 0xb3, 0x08);

CarbonGoldLookAndFeel::CarbonGoldLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, carbonBg);
    setColour(juce::TextButton::buttonColourId, slatePanel);
    setColour(juce::TextButton::textColourOffId, goldAccent);
    setColour(juce::Label::textColourId, goldAccent);
}

void CarbonGoldLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto baseColor = shouldDrawButtonAsDown ? slatePanel.darker(0.3f)
                   : shouldDrawButtonAsHighlighted ? slatePanel.brighter(0.2f)
                   : backgroundColour;

    g.setColour(baseColor);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(shouldDrawButtonAsHighlighted ? goldAccent : goldAccent.withAlpha(0.6f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void CarbonGoldLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPosProportional, float rotaryStartAngle,
                                              float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused(slider);
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto center = bounds.getCentre();

    // Background track
    g.setColour(slatePanel);
    g.fillEllipse(bounds);

    g.setColour(goldAccent.withAlpha(0.4f));
    g.drawEllipse(bounds, 2.0f);

    // Dynamic Arc
    juce::Path arc;
    float rArc = static_cast<float>(radius - 4.0f);
    arc.addCentredArc(center.x, center.y, rArc, rArc, 0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(cyberCyan);
    g.strokePath(arc, juce::PathStrokeType(3.0f));

    // Pointer line
    juce::Path p;
    auto pointerLength = radius * 0.7f;
    p.startNewSubPath(center);
    p.lineTo(static_cast<float>(center.x + pointerLength * std::sin(toAngle)),
             static_cast<float>(center.y - pointerLength * std::cos(toAngle)));
    g.setColour(goldAccent);
    g.strokePath(p, juce::PathStrokeType(2.0f));
}

} // namespace TimeDilationDAW
