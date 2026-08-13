#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace TimeDilationDAW
{

class CarbonGoldLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CarbonGoldLookAndFeel();
    ~CarbonGoldLookAndFeel() override = default;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    // Color definitions
    static const juce::Colour carbonBg;
    static const juce::Colour slatePanel;
    static const juce::Colour cyberCyan;
    static const juce::Colour royalViolet;
    static const juce::Colour goldAccent;
};

} // namespace TimeDilationDAW
