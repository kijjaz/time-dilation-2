#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/RelativisticNodeGraph.h"

namespace TimeDilationDAW
{

struct TimelineClip
{
    int clipId = 0;
    int trackIndex = 0;
    double startTimeSec = 0.0;
    double durationSec = 4.0;
    juce::String name;
    juce::Colour color;
};

class ArrangementTimelineComponent : public juce::Component,
                                       public juce::Timer
{
public:
    ArrangementTimelineComponent(RelativisticNodeGraph& graph);
    ~ArrangementTimelineComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void refreshTimeline();
    void setPlayheadPosition(double timeInSeconds);

    std::function<void(int nodeId)> onInspectNodePatch;

private:
    RelativisticNodeGraph& nodeGraph;
    std::vector<TimelineClip> clips;

    double playheadTimeSec = 0.0;
    double totalDurationSec = 30.0;
    int trackHeight = 60;
    int rulerHeight = 28;
    int trackHeaderWidth = 180;
};

} // namespace TimeDilationDAW
