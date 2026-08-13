#include "ArrangementTimelineComponent.h"
#include "CarbonGoldLookAndFeel.h"

namespace TimeDilationDAW
{

ArrangementTimelineComponent::ArrangementTimelineComponent(RelativisticNodeGraph& graph)
    : nodeGraph(graph)
{
    refreshTimeline();
    startTimerHz(30); // 30 FPS playhead animation
}

ArrangementTimelineComponent::~ArrangementTimelineComponent()
{
    stopTimer();
}

void ArrangementTimelineComponent::refreshTimeline()
{
    clips.clear();
    const auto& nodes = nodeGraph.getNodes();
    int trackIdx = 0;
    for (const auto& node : nodes)
    {
        if (node->getSymbol() == "out~") continue;

        TimelineClip c;
        c.clipId = node->getId();
        c.trackIndex = trackIdx++;
        c.startTimeSec = static_cast<double>((c.clipId * 2) % 12);
        c.durationSec = 6.0;
        c.name = juce::String(node->getLabel());

        if (node->getSymbol() == "osc~") c.color = CarbonGoldLookAndFeel::cyberCyan;
        else if (node->getSymbol() == "pluck~") c.color = CarbonGoldLookAndFeel::goldAccent;
        else if (node->getSymbol() == "ladder~") c.color = CarbonGoldLookAndFeel::royalViolet;
        else if (node->getSymbol() == "time.warp") c.color = juce::Colours::deeppink;
        else c.color = juce::Colours::darkgrey;

        clips.push_back(c);
    }
    repaint();
}

void ArrangementTimelineComponent::setPlayheadPosition(double timeInSeconds)
{
    playheadTimeSec = timeInSeconds;
    repaint();
}

void ArrangementTimelineComponent::timerCallback()
{
    playheadTimeSec += 1.0 / 30.0;
    if (playheadTimeSec > totalDurationSec) playheadTimeSec = 0.0;
    repaint();
}

void ArrangementTimelineComponent::paint(juce::Graphics& g)
{
    g.fillAll(CarbonGoldLookAndFeel::carbonBg);

    int timelineW = getWidth() - trackHeaderWidth;
    if (timelineW <= 0) return;

    // 1. Timeline Ruler Top Header
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(0, 0, getWidth(), rulerHeight);
    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.4f));
    g.drawHorizontalLine(rulerHeight, 0.0f, static_cast<float>(getWidth()));

    // Ruler Ticks (Bar 1..32 / Time 0s..30s)
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    for (int bar = 0; bar <= 16; ++bar)
    {
        float x = trackHeaderWidth + (bar / 16.0f) * timelineW;
        g.drawVerticalLine(static_cast<int>(x), 0.0f, static_cast<float>(rulerHeight));
        g.drawText("Bar " + juce::String(bar * 2 + 1), static_cast<int>(x) + 4, 4, 50, 16, juce::Justification::left);
    }

    // 2. Track Lanes Backgrounds & Headers
    const auto& nodes = nodeGraph.getNodes();
    int trackIdx = 0;
    for (const auto& node : nodes)
    {
        if (node->getSymbol() == "out~") continue;
        int y = rulerHeight + trackIdx * trackHeight;

        // Track Header Box
        g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.05f));
        g.fillRect(0, y, trackHeaderWidth - 2, trackHeight - 2);

        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("Tr " + juce::String(trackIdx + 1) + ": " + juce::String(node->getLabel()), 10, y + 8, trackHeaderWidth - 20, 18, juce::Justification::left);

        g.setColour(juce::Colours::grey);
        g.setFont(10.0f);
        g.drawText("Node #" + juce::String(node->getId()) + " [" + juce::String(node->getSymbol()) + "]", 10, y + 30, trackHeaderWidth - 20, 16, juce::Justification::left);

        // Track Lane Grid Line
        g.setColour(CarbonGoldLookAndFeel::slatePanel.darker(0.3f));
        g.drawHorizontalLine(y + trackHeight - 1, static_cast<float>(trackHeaderWidth), static_cast<float>(getWidth()));

        trackIdx++;
    }

    // 3. Render Audio / MIDI Timeline Clip Blocks
    for (const auto& c : clips)
    {
        int y = rulerHeight + c.trackIndex * trackHeight + 4;
        float startX = trackHeaderWidth + static_cast<float>(c.startTimeSec / totalDurationSec) * timelineW;
        float clipW = static_cast<float>(c.durationSec / totalDurationSec) * timelineW;

        juce::Rectangle<float> clipRect(startX, static_cast<float>(y), clipW, static_cast<float>(trackHeight - 10));

        g.setColour(c.color.withAlpha(0.25f));
        g.fillRoundedRectangle(clipRect, 4.0f);

        g.setColour(c.color);
        g.drawRoundedRectangle(clipRect, 4.0f, 1.5f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(c.name, clipRect.reduced(6.0f), juce::Justification::topLeft, true);
    }

    // 4. Moving Playhead Scrubber Line
    float playheadX = trackHeaderWidth + static_cast<float>(playheadTimeSec / totalDurationSec) * timelineW;
    g.setColour(CarbonGoldLookAndFeel::goldAccent);
    g.drawVerticalLine(static_cast<int>(playheadX), 0.0f, static_cast<float>(getHeight()));

    // Playhead Header Triangle
    juce::Path p;
    p.addTriangle(playheadX - 6.0f, 0.0f, playheadX + 6.0f, 0.0f, playheadX, 10.0f);
    g.fillPath(p);
}

void ArrangementTimelineComponent::resized()
{
}

} // namespace TimeDilationDAW
