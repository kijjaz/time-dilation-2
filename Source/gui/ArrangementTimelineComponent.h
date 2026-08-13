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

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void refreshTimeline();
    void setPlayheadPosition(double timeInSeconds);
    void togglePlayback();
    void rewindToStart();

    std::function<void(int nodeId)> onInspectNodePatch;

private:
    RelativisticNodeGraph& nodeGraph;
    std::vector<TimelineClip> clips;

    // Transport & Playback State
    bool isTimelinePlaying = false; // NON-AUTOPLAY DEFAULT!
    bool isLoopEnabled = true;

    double playheadTimeSec = 0.0;
    double loopStartSec = 0.0;
    double loopEndSec = 16.0;
    bool isSettingLoop = false;

    double totalDurationSec = 30.0;
    int trackHeight = 60;
    int rulerHeight = 32;
    int transportBarHeight = 32;
    int trackHeaderWidth = 180;

    // Transport UI Controls
    juce::TextButton playStopButton{ "▶ PLAY" };
    juce::TextButton rewindButton{ "⏮ REWIND" };
    juce::TextButton loopButton{ "🔁 LOOP ON" };
    juce::Label timeDisplayLabel{ "TimeDisplay", "Bar 1.1 — 00:00.00" };
};

} // namespace TimeDilationDAW
