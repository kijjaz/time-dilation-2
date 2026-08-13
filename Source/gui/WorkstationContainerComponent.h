#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../dsp/RelativisticNodeGraph.h"
#include "RelativisticCanvasComponent.h"
#include "OscilloscopeComponent.h"
#include "NodeInspectorComponent.h"
#include "TrackViewComponent.h"
#include "ArrangementTimelineComponent.h"
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
    NodeInspectorComponent nodeInspectorComponent;

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

    // View Mode Toggle Buttons
    juce::TextButton trackViewButton{ "Arrangement" };
    juce::TextButton canvasViewButton{ "Modular Canvas" };
    juce::TextButton dualViewButton{ "Dual View Split" };

    // Top Header UI
    juce::TextButton playButton{ "DSP ON" };
    juce::TextButton stopButton{ "DSP OFF" };
    juce::Slider masterDilationSlider;
    juce::Label masterDilationLabel;
    juce::Label latencyLabel;
    juce::Label titleLabel;

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

    bool isPlaying = false; // Audio OFF by default when app opens!
    int startupMuteBlocks = 20; // 200ms silent hardware startup gate
    int nextNodeId = 1;

    int inspectorWidth = 260;
    bool isDraggingDivider = false;

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;

    void setupDefaultPatch();
    void showAudioSettingsWindow();
};

} // namespace TimeDilationDAW
