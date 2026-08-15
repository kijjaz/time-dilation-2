#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/RelativisticNodeGraph.h"
#include "../dsp/RelativisticSoundNodes.h"
#include "../dsp/AudioInputRouter.h"
#include "../dsp/TidalPatternEngine.h"
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

enum class RecordingQuantizeMode
{
    None,        // Unquantized (exact punch-in/out)
    Sixteenth,   // 1/16 Note
    Eighth,      // 1/8 Note
    Beat,        // 1 Beat (1/4 Note)
    HalfBar,     // 2 Beats (1/2 Bar)
    Bar          // 1 Bar (4 Beats in 4/4)
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

    // TidalCycles-Style Customizable Pattern & Nested Subdivisions
    std::string tidalPattern = "[60 [62 64] 67 [69 71 72]]";
    std::vector<TidalEvent> cachedEvents;

    void updateTidalPattern(const std::string& pat)
    {
        tidalPattern = pat;
        auto ast = TidalParser::parse(tidalPattern);
        cachedEvents.clear();
        if (ast)
        {
            ast->query(0.0, 1.0, 0, cachedEvents);
        }
    }

    // Audio Sample Clip Data
    std::string sampleTableName;

    // Pattern Data (Notes & Gates fallback)
    std::vector<int> stepPitches{ 60, 62, 64, 65, 67, 69, 71, 72 };
    std::vector<bool> stepGates{ true, false, true, false, true, false, true, false };
    std::vector<float> stepVelocities{ 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f };

    // Automation Data (Breakpoints: normalized offset time 0.0 to 1.0, value 0.0 to 1.0)
    std::vector<std::pair<double, float>> automationPoints{ { 0.0, 0.2f }, { 0.5, 0.85f }, { 1.0, 0.4f } };
};

struct TimelineTrackInfo
{
    int trackIndex = 0;
    juce::String name = "Track 1";
    ClipType defaultClipType = ClipType::Pattern;
    bool isArmed = false;
    AudioInputSource inputSource;
    RecordingQuantizeMode quantizeOverride = RecordingQuantizeMode::Bar;
    bool useTrackQuantizeOverride = false;
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
    void setClipTidalPattern(int clipId, const std::string& pat);

    // TidalCycles-Style Transformation Macros & Graphical Operations
    void applySubdivisionMacro(int division);
    void applyEuclideanMacro(int k, int n);
    void applyStackMacro();
    void applyAlternateMacro();
    void applySpeedMacro(double mult);
    void applyDegradeMacro();
    void commitTidalPattern();

    // Graphical Block, Bracket & Stack Operations
    void wrapSelectedBlockInBrackets();
    void subdivideSelectedBlock(int count);
    void unwrapSelectedBlock();
    void shiftSelectedBlockPitch(int semitones);
    void toggleSelectedBlockRest();
    void showAddStackMenu();
    void addStackLayer(const juce::String& layerPattern);
    void deleteStackLayer(int channelIndex);
    void startInlineBlockEditing(TimelineClip& clip, int eventIdx, const juce::Rectangle<float>& blockBounds);
    void commitInlineBlockEditing();

    // Message Event Operations
    void addMessageEvent(int targetNodeId, int trackIdx, double timeSec, const juce::String& msgText);
    void deleteMessageEvent(int eventId);
    void spawnEventEditor(int targetNodeId, int trackIdx, double timeSec, int existingEventId = -1);
    void commitEventEditor();

    const std::vector<TimelineClip>& getClips() const { return clips; }
    const std::vector<TimelineMessageEvent>& getMessageEvents() const { return messageEvents; }
    void clearMessageEvents() { messageEvents.clear(); selectedEventId = -1; repaint(); }

    // Track Header, Arming & Input Routing Operations
    void setTrackArmed(int trackIdx, bool armed);
    bool isTrackArmed(int trackIdx) const;
    void setTrackInputSource(int trackIdx, const AudioInputSource& source);
    AudioInputSource getTrackInputSource(int trackIdx) const;
    void showTrackInputMenu(int trackIdx);

    // Recording Quantization Controls
    void setRecordingQuantizeMode(RecordingQuantizeMode mode);
    RecordingQuantizeMode getRecordingQuantizeMode() const { return globalRecQuantize; }
    void showRecordingQuantizeMenu();
    static double quantizeTime(double timeSec, RecordingQuantizeMode mode, double bpm);
    static double quantizeDuration(double durationSec, RecordingQuantizeMode mode, double bpm);
    static juce::String getQuantizeModeName(RecordingQuantizeMode mode);

    const std::vector<TimelineTrackInfo>& getTracks() const { return tracks; }

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
    void drawTidalSubdivisionBlocks(juce::Graphics& g, const TimelineClip& clip, const juce::Rectangle<float>& clipRect);
    void drawAutomationCurves(juce::Graphics& g, const TimelineClip& clip, const juce::Rectangle<float>& clipRect);
    void drawAudioWaveformClip(juce::Graphics& g, const TimelineClip& clip, const juce::Rectangle<float>& clipRect);

    RelativisticNodeGraph& nodeGraph;
    std::vector<TimelineTrackInfo> tracks;
    std::vector<TimelineClip> clips;
    std::vector<TimelineMessageEvent> messageEvents;

    // Live Track Recording buffers
    std::map<int, std::vector<float>> trackRecordingBuffers;
    double recordingStartPlayheadTime = 0.0;
    bool wasPlayingPreviousPass = false;

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

    // Piano Roll & Tidal Pattern Drawer State
    bool isPianoRollVisible = true;
    int pianoRollHeight = 185;
    int selectedStepIndex = -1;
    int selectedTidalEventIdx = -1;
    int selectedTidalChannel = 0;
    int draggingTidalEventIdx = -1;
    float tidalDragStartY = 0.0f;
    int tidalDragStartPitch = 60;

    juce::TextEditor eventEditor;
    juce::TextEditor inlineBlockEditor;
    bool isInlineEditingBlock = false;
    int inlineEditingEventIdx = -1;

    // Tidal Pattern Editor UI Controls
    juce::TextEditor tidalPatternEditor;
    juce::TextButton wrapBracketBtn{ "[ ... ]" };
    juce::TextButton subdivideBtn{ "[a b] /2" };
    juce::TextButton tripletBtn{ "[/3]" };
    juce::TextButton quadBtn{ "[/4]" };
    juce::TextButton unwrapBtn{ "Unwrap" };
    juce::TextButton stackBtn{ "+ Stack Poly (,)" };
    juce::TextButton euclidBtn{ "Euclid (3,8)" };
    juce::TextButton alternateBtn{ "<a b> Alt" };
    juce::TextButton speed2Btn{ "*2" };
    juce::TextButton degradeBtn{ "? Prob" };
    juce::TextButton restBtn{ "~ Rest" };
    juce::TextButton pitchUpBtn{ "+1" };
    juce::TextButton pitchDownBtn{ "-1" };
    juce::TextButton octUpBtn{ "+12" };
    juce::TextButton octDownBtn{ "-12" };
    juce::TextButton applyPatternBtn{ "APPLY" };
    juce::TextButton tidalHelpBtn{ "[?] HELP" };

    void showTidalHelpModal();
    void showTidalBlockContextMenu(TimelineClip& clip, int eventIdx);

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
    juce::TextButton recQuantizeButton{ "⏱ Q: 1 BAR" };
    juce::TextButton togglePianoRollBtn{ "TIDAL DRAWER" };
    juce::Label timeDisplayLabel{ "TimeDisplay", "Bar 1.1 | 00:00.00" };

    RecordingQuantizeMode globalRecQuantize = RecordingQuantizeMode::Bar;
};

} // namespace TimeDilationDAW
