#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../dsp/RelativisticNodeGraph.h"
#include "RelativisticCanvasComponent.h"
#include "OscilloscopeComponent.h"
#include "NodeInspectorComponent.h"
#include "TrackViewComponent.h"
#include "ArrangementTimelineComponent.h"
#include "ConsolePanelComponent.h"
#include "CarbonGoldLookAndFeel.h"

namespace TimeDilationDAW
{

class WorkstationContainerComponent : public juce::Component,
                                       public juce::AudioSource
{
public:
    explicit WorkstationContainerComponent(bool enableAudioHardware = true);
    ~WorkstationContainerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void toggleConsole();

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    // AudioSource methods
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    RelativisticNodeGraph& getNodeGraph() { return nodeGraph; }
    void setIsPlaying(bool play) { isPlaying = play; }

private:
    CarbonGoldLookAndFeel lookAndFeel;
    RelativisticNodeGraph nodeGraph;
    RelativisticCanvasComponent canvasComponent;
    TrackViewComponent trackViewComponent;
    ArrangementTimelineComponent arrangementTimelineComponent;
    OscilloscopeComponent oscilloscopeComponent;
    juce::Viewport inspectorViewport;
    NodeInspectorComponent nodeInspectorComponent;
    ConsolePanelComponent consolePanel;
    bool isConsoleVisible = false;
    int consoleHeight = 150;

    enum class ViewMode { TrackView, ModularCanvas, DualViewSplit };
    ViewMode currentViewMode = ViewMode::ModularCanvas;

class FlatMenuButton : public juce::TextButton
{
public:
    FlatMenuButton(const juce::String& name) : juce::TextButton(name) {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (shouldDrawButtonAsDown)
        {
            g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.25f));
            g.fillRoundedRectangle(bounds, 3.0f);
        }
        else if (shouldDrawButtonAsHighlighted)
        {
            g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.3f));
            g.fillRoundedRectangle(bounds, 3.0f);
        }

        g.setColour(shouldDrawButtonAsHighlighted ? CarbonGoldLookAndFeel::goldAccent : juce::Colours::lightgrey);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred, true);
    }
};

// Master DAW AudioPlayHead implementation using JUCE built-in PositionInfo
class WorkstationPlayHead : public juce::AudioPlayHead
{
public:
    WorkstationPlayHead() = default;

    void setBpm(double bpmVal) { bpm = bpmVal; }
    double getBpm() const { return bpm; }
    void setTimeSignature(int num, int den) { numBeats = num; beatValue = den; }
    int getNumBeats() const { return numBeats; }
    int getBeatValue() const { return beatValue; }
    void setIsPlaying(bool playing) { isPlayingVal = playing; }
    void setIsLooping(bool looping) { isLoopingVal = looping; }
    void setLoopPoints(double startPpq, double endPpq) { loopStartPpqVal = startPpq; loopEndPpqVal = endPpq; }
    void setPpqPosition(double ppq) { currentPpq = ppq; }
    double getPpqPosition() const { return currentPpq; }

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm(bpm);
        juce::AudioPlayHead::TimeSignature ts;
        ts.numerator = numBeats;
        ts.denominator = beatValue;
        info.setTimeSignature(ts);
        info.setIsPlaying(isPlayingVal);
        info.setIsLooping(isLoopingVal);
        info.setPpqPosition(currentPpq);

        double ppqPerBar = static_cast<double>(numBeats) * (4.0 / static_cast<double>(beatValue));
        double lastBarPpq = std::floor(currentPpq / std::max(1.0, ppqPerBar)) * ppqPerBar;
        info.setPpqPositionOfLastBarStart(lastBarPpq);

        if (isLoopingVal)
        {
            juce::AudioPlayHead::LoopPoints lp;
            lp.ppqStart = loopStartPpqVal;
            lp.ppqEnd = loopEndPpqVal;
            info.setLoopPoints(lp);
        }
        return info;
    }

private:
    double bpm = 120.0;
    int numBeats = 4;
    int beatValue = 4;
    bool isPlayingVal = false;
    bool isLoopingVal = false;
    double currentPpq = 0.0;
    double loopStartPpqVal = 0.0;
    double loopEndPpqVal = 32.0;
};

    // View Mode Toggle Buttons
    juce::TextButton trackViewButton{ "Arrangement" };
    juce::TextButton canvasViewButton{ "Modular Canvas" };
    juce::TextButton dualViewButton{ "Dual View Split" };

    // Top Header UI
    juce::TextButton playButton{ "DSP ON" };
    juce::TextButton stopButton{ "DSP OFF" };

    // JUCE Tempo & Time Signature Controls
    juce::Slider bpmSlider;
    juce::Label bpmLabel{ "BPMLabel", "BPM" };

    juce::ComboBox timeSigCombo;
    juce::Label timeSigLabel{ "TimeSigLabel", "Sig" };

    juce::Slider masterDilationSlider;
    juce::Label masterDilationLabel;
    juce::Label latencyLabel;
    juce::Label titleLabel;

    WorkstationPlayHead masterPlayHead;

    // Sleek Desktop Top Menu Bar
    FlatMenuButton fileMenuButton{ "File" };
    FlatMenuButton editMenuButton{ "Edit" };
    FlatMenuButton putMenuButton{ "Put" };
    FlatMenuButton viewMenuButton{ "View" };
    FlatMenuButton workflowMenuButton{ "Workflows" };
    FlatMenuButton helpMenuButton{ "Help" };

    void setupComposerTemplate();
    void setupSoundDesignerTemplate();
    void setupFilmSciFiTemplate();
    void setupExperimentalistTemplate();

    bool isPlaying = true; // Audio DSP Engine ON by default!
    int startupMuteBlocks = 5; // Clean hardware startup gate
    int nextNodeId = 1;

    int inspectorWidth = 260;
    bool isDraggingDivider = false;

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;

    bool keyPressed(const juce::KeyPress& key) override;

public:
    void newPatch();
    void savePatch();
    void savePatchAs();
    void loadPatchFromFile(const juce::File& fileToLoad);

    juce::File currentPatchFile;
    std::unique_ptr<juce::FileChooser> activeFileChooser;

    void setupDefaultPatch();
    void loadExampleFullEnsemble();
    void loadExampleDrumGroove();
    void showAudioSettingsWindow();
};

} // namespace TimeDilationDAW
