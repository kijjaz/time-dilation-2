#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/RelativisticNodeGraph.h"
#include <vector>
#include <string>

namespace TimeDilationDAW
{

enum class ClipType
{
    Pattern,
    Automation,
    AudioSample
};

struct TimelineClip
{
    int clipId = 0;
    int trackIndex = 0;
    ClipType type = ClipType::Pattern;
    double startTimeSec = 0.0;
    double durationSec = 4.0;
    juce::String name;
    juce::Colour color;

    // Pattern Data (Notes & Gates)
    std::vector<int> stepPitches{ 60, 62, 64, 65, 67, 69, 71, 72 };
    std::vector<bool> stepGates{ true, false, true, false, true, false, true, false };
    std::vector<float> stepVelocities{ 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f };

    // Automation Data (Breakpoints: normalized offset time 0.0 to 1.0, value 0.0 to 1.0)
    std::vector<std::pair<double, float>> automationPoints{ { 0.0, 0.2f }, { 0.5, 0.85f }, { 1.0, 0.4f } };
};

struct TimelineMessageEvent
{
    int eventId = 0;
    int targetNodeId = 0;
    int trackIndex = 0;
    double timeSec = 0.0;
    juce::String messageText;
    juce::Colour color;
    bool triggeredInCurrentPass = false;
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
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void refreshTimeline();
    void setPlayheadPosition(double timeInSeconds);
    void togglePlayback();
    void rewindToStart();

    // Clip Operations
    void addClip(int trackIdx, double startSec, double durationSec, const juce::String& name, ClipType type = ClipType::Pattern);
    void deleteClip(int clipId);
    void duplicateSelectedClip();
    void splitClipAtPlayhead();

    // Message Event Operations
    void addMessageEvent(int targetNodeId, int trackIdx, double timeSec, const juce::String& msgText);
    void deleteMessageEvent(int eventId);
    void spawnEventEditor(int targetNodeId, int trackIdx, double timeSec, int existingEventId = -1);
    void commitEventEditor();

    const std::vector<TimelineClip>& getClips() const { return clips; }
    const std::vector<TimelineMessageEvent>& getMessageEvents() const { return messageEvents; }
    void clearMessageEvents() { messageEvents.clear(); selectedEventId = -1; repaint(); }

    double getLoopStartSec() const { return loopStartSec; }
    double getLoopEndSec() const { return loopEndSec; }
    bool isLoopActive() const { return isLoopEnabled; }
    void setLoopRange(double startSec, double endSec, bool enableLoop)
    {
        loopStartSec = startSec;
        loopEndSec = endSec;
        isLoopEnabled = enableLoop;
        loopButton.setButtonText(isLoopEnabled ? "LOOP ON" : "LOOP OFF");
        repaint();
    }

    std::function<void(int nodeId)> onInspectNodePatch;
    std::function<void(bool isPlaying)> onPlaybackToggled;

private:
    void drawPianoRollDrawer(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawAutomationCurves(juce::Graphics& g, const TimelineClip& clip, const juce::Rectangle<float>& clipRect);

    RelativisticNodeGraph& nodeGraph;
    std::vector<TimelineClip> clips;
    std::vector<TimelineMessageEvent> messageEvents;

    int selectedClipId = -1;
    int draggingClipId = -1;
    double dragStartClipTime = 0.0;
    bool isResizingClipEnd = false;

    int selectedEventId = -1;
    int editingEventId = -1;
    int editingTargetNodeId = -1;
    int editingTrackIndex = -1;
    double editingTimeSec = 0.0;
    bool isEditingEvent = false;
    bool isDraggingEvent = false;

    // Piano Roll & Step Grid Drawer State
    bool isPianoRollVisible = true;
    int pianoRollHeight = 160;
    int selectedStepIndex = -1;

    juce::TextEditor eventEditor;

    // Transport & Playback State
    bool isTimelinePlaying = false;
    bool isLoopEnabled = true;
    double bpm = 120.0;

    double playheadTimeSec = 0.0;
    double lastPlayheadTimeSec = 0.0;
    double loopStartSec = 0.0;
    double loopEndSec = 16.0;
    bool isSettingLoop = false;
    bool isDraggingPlayhead = false;

    double totalDurationSec = 32.0;
    int trackHeight = 55;
    int rulerHeight = 32;
    int transportBarHeight = 34;
    int trackHeaderWidth = 180;

    // Transport UI Controls
    juce::TextButton playStopButton{ "PLAY" };
    juce::TextButton rewindButton{ "REWIND" };
    juce::TextButton loopButton{ "LOOP ON" };
    juce::TextButton addClipButton{ "+ ADD CLIP" };
    juce::TextButton togglePianoRollBtn{ "PIANO ROLL" };
    juce::Label timeDisplayLabel{ "TimeDisplay", "Bar 1.1 | 00:00.00" };
};

} // namespace TimeDilationDAW
