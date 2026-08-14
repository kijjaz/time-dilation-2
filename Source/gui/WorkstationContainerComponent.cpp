#include "WorkstationContainerComponent.h"
#include "../dsp/RelativisticNodeFactory.h"

namespace TimeDilationDAW
{

WorkstationContainerComponent::WorkstationContainerComponent(bool enableAudioHardware)
    : canvasComponent(nodeGraph), trackViewComponent(nodeGraph), arrangementTimelineComponent(nodeGraph)
{
    setLookAndFeel(&lookAndFeel);

    // Title
    titleLabel.setText("TIME DILATION DAW 2", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(titleLabel);

    // Play/Stop
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);

    playButton.onClick = [this]() { isPlaying = true; };
    stopButton.onClick = [this]() { isPlaying = false; };

    // View Mode Toggle Buttons
    addAndMakeVisible(trackViewButton);
    addAndMakeVisible(canvasViewButton);
    addAndMakeVisible(dualViewButton);

    trackViewButton.onClick = [this]() {
        currentViewMode = ViewMode::TrackView;
        trackViewComponent.refreshTracks();
        arrangementTimelineComponent.refreshTimeline();
        resized();
        repaint();
    };

    canvasViewButton.onClick = [this]() {
        currentViewMode = ViewMode::ModularCanvas;
        resized();
        repaint();
    };

    dualViewButton.onClick = [this]() {
        currentViewMode = ViewMode::DualViewSplit;
        trackViewComponent.refreshTracks();
        arrangementTimelineComponent.refreshTimeline();
        resized();
        repaint();
    };

    // Attach master JUCE AudioPlayHead to node graph
    nodeGraph.setAudioPlayHead(&masterPlayHead);

    // BPM Slider
    bpmSlider.setRange(-100000.0, 100000.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::IncDecButtons);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 45, 20);
    bpmSlider.onValueChange = [this]() {
        masterPlayHead.setBpm(bpmSlider.getValue());
    };
    addAndMakeVisible(bpmSlider);

    bpmLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    bpmLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(bpmLabel);

    // Time Signature Combo
    timeSigCombo.addItem("4/4", 1);
    timeSigCombo.addItem("3/4", 2);
    timeSigCombo.addItem("6/8", 3);
    timeSigCombo.addItem("7/8", 4);
    timeSigCombo.addItem("5/4", 5);
    timeSigCombo.setSelectedId(1, juce::dontSendNotification);
    timeSigCombo.onChange = [this]() {
        int id = timeSigCombo.getSelectedId();
        if (id == 1) masterPlayHead.setTimeSignature(4, 4);
        else if (id == 2) masterPlayHead.setTimeSignature(3, 4);
        else if (id == 3) masterPlayHead.setTimeSignature(6, 8);
        else if (id == 4) masterPlayHead.setTimeSignature(7, 8);
        else if (id == 5) masterPlayHead.setTimeSignature(5, 4);
    };
    addAndMakeVisible(timeSigCombo);

    timeSigLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    timeSigLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(timeSigLabel);

    latencyLabel.setText("LATENCY: 5.3 ms (Real-Time)", juce::dontSendNotification);
    latencyLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    latencyLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::cyberCyan);
    addAndMakeVisible(latencyLabel);

    addAndMakeVisible(oscilloscopeComponent);

    // Sleek Desktop Top Menu Bar
    addAndMakeVisible(fileMenuButton);
    addAndMakeVisible(editMenuButton);
    addAndMakeVisible(putMenuButton);
    addAndMakeVisible(viewMenuButton);
    addAndMakeVisible(workflowMenuButton);
    addAndMakeVisible(helpMenuButton);

    fileMenuButton.onClick = [this]() {
        juce::PopupMenu m;
        m.addItem(1, "New Patch (Cmd+N)");
        m.addItem(2, "Open Patch (.pdil)... (Cmd+O)");
        m.addSeparator();

        juce::PopupMenu examplesMenu;
        examplesMenu.addItem(101, "01: Full Workstation Ensemble (Synth + Drums + FX)");
        examplesMenu.addItem(102, "02: Analog Drum Machine & Groove (Focused Kick/Snare/Hat)");
        m.addSubMenu("Examples & Presets", examplesMenu);

        m.addSeparator();
        m.addItem(3, "Save Patch (.pdil) (Cmd+S)");
        m.addItem(4, "Save As... (Cmd+Shift+S)");
        m.addSeparator();
        m.addItem(5, "Audio Settings (Interface, Sample Rate, Buffer Size)...");
        m.addSeparator();
        m.addItem(6, "Quit");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&fileMenuButton), [this](int result) {
            if (result == 1) newPatch();
            else if (result == 2) loadPatchFromFile(juce::File{});
            else if (result == 3) savePatch();
            else if (result == 4) savePatchAs();
            else if (result == 5) showAudioSettingsWindow();
            else if (result == 6) juce::JUCEApplication::getInstance()->systemRequestedQuit();
            else if (result == 101) loadExampleFullEnsemble();
            else if (result == 102) loadExampleDrumGroove();
        });
    };

    editMenuButton.onClick = [this]() {
        juce::PopupMenu m;
        m.addItem(1, "Undo (Cmd+Z)", false);
        m.addItem(2, "Redo (Cmd+Shift+Z)", false);
        m.addSeparator();
        m.addItem(3, "Delete Selected (Esc / Backspace)");
        m.addItem(4, "Clear All Nodes");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&editMenuButton), [this](int result) {
            if (result == 4) { nodeGraph.clearGraph(); canvasComponent.repaint(); trackViewComponent.refreshTracks(); }
        });
    };

    putMenuButton.onClick = [this]() {
        juce::PopupMenu m;
        m.addItem(1, "Object Box (Cmd+1 / Enter)");
        m.addSeparator();
        m.addItem(2, "osc~ Atomic Oscillator");
        m.addItem(3, "osc.patch~ Inspectable Composite Oscillator");
        m.addItem(4, "pluck~ Karplus-Strong String");
        m.addItem(5, "ladder~ Moog 4-Pole VA Ladder Filter");
        m.addItem(6, "drive~ WaveShaper Distortion");
        m.addItem(7, "time.warp Relativistic Time Dilation");
        m.addItem(8, "seq Step Sequencer");
        m.addItem(9, "patch~ Composite Sub-Graph");
        m.addSeparator();
        m.addItem(10, "print Message & Number Console Logger");
        m.addItem(11, "print~ Audio Signal & Envelope Console Probe");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&putMenuButton), [this](int result) {
            if (result == 1) { canvasComponent.spawnObjectEditorAt({ 200.0f, 200.0f }); }
            else if (result == 2) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "osc~ sin"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 3) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "osc.patch~ sin"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 4) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "pluck~ 220"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 5) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "ladder~ 1000 0.5"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 6) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "drive~ 2.5"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 7) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.warp 2.0"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 8) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "seq 60 62 64 67"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 9) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "patch~ synth.voice~"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 10) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "print"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 11) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "print~"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            trackViewComponent.refreshTracks();
        });
    };

    viewMenuButton.onClick = [this]() {
        juce::PopupMenu m;
        m.addItem(1, "Recenter Canvas (Cmd+0)");
        m.addItem(2, "Toggle Oscilloscope", true, oscilloscopeComponent.isVisible());
        m.addSeparator();
        m.addItem(3, "Toggle Terminal Console (Cmd+K)", true, isConsoleVisible);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&viewMenuButton), [this](int result) {
            if (result == 1) { canvasComponent.recenterView(); }
            else if (result == 2) { oscilloscopeComponent.setVisible(!oscilloscopeComponent.isVisible()); }
            else if (result == 3) { toggleConsole(); }
        });
    };

    workflowMenuButton.onClick = [this]() {
        juce::PopupMenu m;
        m.addItem(1, "01: Full Workstation Ensemble (Synth + Drums + FX)");
        m.addItem(2, "02: Analog Drum Machine & Groove (Kick, Snare, Hi-Hat)");
        m.addSeparator();
        m.addItem(3, "Composer / Musician Mode (Synth Lead + Pluck String)");
        m.addItem(4, "Sound Designer Mode (Moog Ladder Filter + WaveShaper)");
        m.addItem(5, "Film & Game Sci-Fi Mode (Relativistic Doppler Wormhole)");
        m.addItem(6, "Experimentalist Mode (Tarjan Feedback Chaos Loop)");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&workflowMenuButton), [this](int result) {
            if (result == 1) loadExampleFullEnsemble();
            else if (result == 2) loadExampleDrumGroove();
            else if (result == 3) setupComposerTemplate();
            else if (result == 4) setupSoundDesignerTemplate();
            else if (result == 5) setupFilmSciFiTemplate();
            else if (result == 6) setupExperimentalistTemplate();
            trackViewComponent.refreshTracks();
            canvasComponent.repaint();
        });
    };

    helpMenuButton.onClick = [this]() {
        juce::PopupMenu m;
        m.addItem(1, "Node Symbol Reference (35+ Symbols)");
        m.addItem(2, "Relativistic Time Math Guide");
        m.addSeparator();
        m.addItem(3, "About Time Dilation DAW 2");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&helpMenuButton), nullptr);
    };

    addAndMakeVisible(canvasComponent);
    addAndMakeVisible(trackViewComponent);
    addAndMakeVisible(arrangementTimelineComponent);
    addAndMakeVisible(inspectorViewport);
    inspectorViewport.setViewedComponent(&nodeInspectorComponent, false);
    inspectorViewport.setScrollBarsShown(true, false, true, false);
    inspectorViewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, CarbonGoldLookAndFeel::goldAccent.withAlpha(0.6f));

    addChildComponent(consolePanel);
    consolePanel.onCloseRequested = [this]() {
        isConsoleVisible = false;
        consolePanel.setVisible(false);
        resized();
        repaint();
    };

    trackViewComponent.onInspectNodePatch = [this](int nodeId) {
        juce::ignoreUnused(nodeId);
        currentViewMode = ViewMode::ModularCanvas;
        resized();
        repaint();
    };

    canvasComponent.onNodeSelected = [this](std::shared_ptr<RelativisticNode> node) {
        nodeInspectorComponent.setSelectedNode(node);
    };

    nodeInspectorComponent.onSpawnMessageBox = [this](int targetNodeId, const std::string& msgText) {
        canvasComponent.spawnMessageBoxForNode(targetNodeId, msgText);
    };

    arrangementTimelineComponent.onPlaybackToggled = [this](bool play) {
        isPlaying = play;
    };

    setupDefaultPatch();
    arrangementTimelineComponent.refreshTimeline();

    if (enableAudioHardware)
    {
        deviceManager.initialiseWithDefaultDevices(0, 2);
        audioSourcePlayer.setSource(this);
        deviceManager.addAudioCallback(&audioSourcePlayer);
    }
}

WorkstationContainerComponent::~WorkstationContainerComponent()
{
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    setLookAndFeel(nullptr);
}

void WorkstationContainerComponent::setupDefaultPatch()
{
    loadExampleDrumGroove();
}

void WorkstationContainerComponent::loadExampleFullEnsemble()
{
    nodeGraph.clearGraph();

    // Row 1: Relativistic Clock & Melodic Synthesizer Voice
    auto lfoNode = RelativisticNodeFactory::createNode(1, "time.lfo 0.5 0.5");
    lfoNode->xPos = 40; lfoNode->yPos = 40;

    auto seqNode = RelativisticNodeFactory::createNode(2, "seq notes 48 55 58 60 62 65 67 70");
    seqNode->xPos = 190; seqNode->yPos = 40;

    auto mtofNode = RelativisticNodeFactory::createNode(3, "mtof~ 48");
    mtofNode->xPos = 340; mtofNode->yPos = 40;

    auto oscNode = RelativisticNodeFactory::createNode(4, "osc~ saw");
    oscNode->setOutputVolume(0.85f);
    oscNode->xPos = 490; oscNode->yPos = 40;

    auto ladderNode = RelativisticNodeFactory::createNode(5, "ladder~ 1800 0.5");
    ladderNode->xPos = 640; ladderNode->yPos = 40;

    auto delayNode = RelativisticNodeFactory::createNode(6, "delay~ 0.375 0.42");
    delayNode->setOutputVolume(0.65f);
    delayNode->xPos = 790; delayNode->yPos = 40;

    // Row 2: Analog Rhythm Section (Sequencers + Drum Synths)
    auto seqKickNode = RelativisticNodeFactory::createNode(7, "seq 1 0 0 0 1 0 0 0 1 0 0 1 1 0 0 0");
    seqKickNode->setLabel("seq.kick");
    seqKickNode->xPos = 40; seqKickNode->yPos = 200;

    auto kickNode = RelativisticNodeFactory::createNode(8, "kick~ 55 0.35");
    kickNode->setOutputVolume(0.95f);
    kickNode->xPos = 190; kickNode->yPos = 200;

    auto seqSnareNode = RelativisticNodeFactory::createNode(9, "seq 0 0 0 0 1 0 0 0 0 0 0 0 1 0 1 0");
    seqSnareNode->setLabel("seq.snare");
    seqSnareNode->xPos = 340; seqSnareNode->yPos = 200;

    auto snareNode = RelativisticNodeFactory::createNode(10, "snare~ 185 0.70 0.28");
    snareNode->setOutputVolume(0.85f);
    snareNode->xPos = 490; snareNode->yPos = 200;

    auto seqHatNode = RelativisticNodeFactory::createNode(11, "seq 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1");
    seqHatNode->setLabel("seq.hihat");
    seqHatNode->xPos = 640; seqHatNode->yPos = 200;

    auto hihatNode = RelativisticNodeFactory::createNode(12, "hihat~ 0.08");
    hihatNode->setOutputVolume(0.70f);
    hihatNode->xPos = 790; hihatNode->yPos = 200;

    // Row 3: Space FX & Stereo Master
    auto reverbNode = RelativisticNodeFactory::createNode(13, "reverb~ 0.75 0.4 0.35");
    reverbNode->setOutputVolume(0.55f);
    reverbNode->xPos = 340; reverbNode->yPos = 360;

    auto outNode = RelativisticNodeFactory::createNode(14, "out~ master");
    outNode->setOutputVolume(1.0f);
    outNode->xPos = 520; outNode->yPos = 360;

    if (auto out = std::dynamic_pointer_cast<OutNode>(outNode))
    {
        out->onPlaybackStateChanged = [this](bool play) {
            isPlaying = play;
        };
    }

    nodeGraph.addNode(lfoNode);
    nodeGraph.addNode(seqNode);
    nodeGraph.addNode(mtofNode);
    nodeGraph.addNode(oscNode);
    nodeGraph.addNode(ladderNode);
    nodeGraph.addNode(delayNode);
    nodeGraph.addNode(seqKickNode);
    nodeGraph.addNode(kickNode);
    nodeGraph.addNode(seqSnareNode);
    nodeGraph.addNode(snareNode);
    nodeGraph.addNode(seqHatNode);
    nodeGraph.addNode(hihatNode);
    nodeGraph.addNode(reverbNode);
    nodeGraph.addNode(outNode);

    // Cable Patch Routing:
    // 1. Time LFO -> All Sequencers & Osc~
    nodeGraph.addConnection(1, 1, 2, 1);  // lfo timeOut -> melodic seq timeIn
    nodeGraph.addConnection(1, 1, 4, 1);  // lfo timeOut -> osc~ timeIn
    nodeGraph.addConnection(1, 1, 7, 1);  // lfo timeOut -> kick seq timeIn
    nodeGraph.addConnection(1, 1, 9, 1);  // lfo timeOut -> snare seq timeIn
    nodeGraph.addConnection(1, 1, 11, 1); // lfo timeOut -> hihat seq timeIn

    // 2. Melodic Synth Chain: Seq -> Mtof~ -> Osc~ -> Ladder~ -> Delay~
    nodeGraph.addConnection(2, 1, 3, 1);  // seq note (Outlet 1) -> mtof~ note~ (Inlet 1)
    nodeGraph.addConnection(3, 1, 4, 2);  // mtof~ freq~ (Outlet 1) -> osc~ freq~ (Inlet 2)
    nodeGraph.addConnection(4, 2, 5, 2);  // osc~ out~ (Outlet 2) -> ladder~ in~ (Inlet 2)
    nodeGraph.addConnection(5, 2, 6, 1);  // ladder~ out~ (Outlet 2) -> delay~ in~ (Inlet 1)

    // 3. Rhythm Trigger Chains: Seq Gate -> Drum Synth Trig~
    nodeGraph.addConnection(7, 2, 8, 1);   // seq.kick gate (Outlet 2) -> kick~ trig~ (Inlet 1)
    nodeGraph.addConnection(9, 2, 10, 1);  // seq.snare gate (Outlet 2) -> snare~ trig~ (Inlet 1)
    nodeGraph.addConnection(11, 2, 12, 1); // seq.hihat gate (Outlet 2) -> hihat~ trig~ (Inlet 1)

    // 4. Synth & Snare to Reverb
    nodeGraph.addConnection(6, 1, 13, 1);  // delay~ out~ (Outlet 1) -> reverb~ in1~ (Inlet 1)
    nodeGraph.addConnection(6, 1, 13, 2);  // delay~ out~ (Outlet 1) -> reverb~ in2~ (Inlet 2)
    nodeGraph.addConnection(10, 1, 13, 1); // snare~ out~ (Outlet 1) -> reverb~ in1~ (Inlet 1)
    nodeGraph.addConnection(10, 1, 13, 2); // snare~ out~ (Outlet 1) -> reverb~ in2~ (Inlet 2)

    // 5. Final Summation into Out~ Master (Inlets 1 & 2)
    nodeGraph.addConnection(6, 1, 14, 1);  // delay~ (dry/echo) -> out~ L
    nodeGraph.addConnection(6, 1, 14, 2);  // delay~ (dry/echo) -> out~ R
    nodeGraph.addConnection(8, 1, 14, 1);  // kick~ -> out~ L
    nodeGraph.addConnection(8, 1, 14, 2);  // kick~ -> out~ R
    nodeGraph.addConnection(10, 1, 14, 1); // snare~ -> out~ L
    nodeGraph.addConnection(10, 1, 14, 2); // snare~ -> out~ R
    nodeGraph.addConnection(12, 1, 14, 1); // hihat~ -> out~ L
    nodeGraph.addConnection(12, 1, 14, 2); // hihat~ -> out~ R
    nodeGraph.addConnection(13, 1, 14, 1); // reverb~ out1~ -> out~ L
    nodeGraph.addConnection(13, 2, 14, 2); // reverb~ out2~ -> out~ R

    nextNodeId = 15;
    arrangementTimelineComponent.refreshTimeline();
    canvasComponent.repaint();
    trackViewComponent.refreshTracks();
}

void WorkstationContainerComponent::loadExampleDrumGroove()
{
    nodeGraph.clearGraph();

    // 1. Master Relativistic Clock (time.lfo @ 120 BPM)
    auto lfoNode = RelativisticNodeFactory::createNode(1, "time.lfo 0.5 0.5");
    lfoNode->xPos = 40; lfoNode->yPos = 40;

    // 2. Kick Drum Section: Sequencer + Sub-bass Kick Synth
    auto seqKickNode = RelativisticNodeFactory::createNode(2, "seq 1 0 0 0 1 0 0 0 1 0 0 1 1 0 0 0");
    seqKickNode->setLabel("seq.kick");
    seqKickNode->xPos = 200; seqKickNode->yPos = 40;

    auto kickNode = RelativisticNodeFactory::createNode(3, "kick~ 55 0.35");
    kickNode->setOutputVolume(0.95f);
    kickNode->xPos = 380; kickNode->yPos = 40;

    // 3. Snare Drum Section: Sequencer + Dual-Tone Snare Synth
    auto seqSnareNode = RelativisticNodeFactory::createNode(4, "seq 0 0 0 0 1 0 0 0 0 0 0 0 1 0 1 0");
    seqSnareNode->setLabel("seq.snare");
    seqSnareNode->xPos = 200; seqSnareNode->yPos = 190;

    auto snareNode = RelativisticNodeFactory::createNode(5, "snare~ 185 0.70 0.28");
    snareNode->setOutputVolume(0.85f);
    snareNode->xPos = 380; snareNode->yPos = 190;

    // 4. Hi-Hat Section: Sequencer + Metallic Cluster Hi-Hat Synth
    auto seqHatNode = RelativisticNodeFactory::createNode(6, "seq 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1");
    seqHatNode->setLabel("seq.hihat");
    seqHatNode->xPos = 200; seqHatNode->yPos = 340;

    auto hihatNode = RelativisticNodeFactory::createNode(7, "hihat~ 0.08");
    hihatNode->setOutputVolume(0.70f);
    hihatNode->xPos = 380; hihatNode->yPos = 340;

    // 5. Stereo Master Output Bus
    auto outNode = RelativisticNodeFactory::createNode(8, "out~ master");
    outNode->setOutputVolume(1.0f);
    outNode->xPos = 580; outNode->yPos = 190;

    if (auto out = std::dynamic_pointer_cast<OutNode>(outNode))
    {
        out->onPlaybackStateChanged = [this](bool play) {
            isPlaying = play;
        };
    }

    nodeGraph.addNode(lfoNode);
    nodeGraph.addNode(seqKickNode);
    nodeGraph.addNode(kickNode);
    nodeGraph.addNode(seqSnareNode);
    nodeGraph.addNode(snareNode);
    nodeGraph.addNode(seqHatNode);
    nodeGraph.addNode(hihatNode);
    nodeGraph.addNode(outNode);

    // Cable Patch Routing:
    // Clock -> Sequencers
    nodeGraph.addConnection(1, 1, 2, 1);  // lfo timeOut -> kick seq timeIn
    nodeGraph.addConnection(1, 1, 4, 1);  // lfo timeOut -> snare seq timeIn
    nodeGraph.addConnection(1, 1, 6, 1);  // lfo timeOut -> hihat seq timeIn

    // Sequencer Gate Outlets (Outlet 2) -> Drum Trigger Inlets (Inlet 1)
    nodeGraph.addConnection(2, 2, 3, 1);  // seq.kick gate -> kick~ trig~
    nodeGraph.addConnection(4, 2, 5, 1);  // seq.snare gate -> snare~ trig~
    nodeGraph.addConnection(6, 2, 7, 1);  // seq.hihat gate -> hihat~ trig~

    // Drum Audio Outlets (Outlet 1) -> Out~ Master (Inlets 1 & 2)
    nodeGraph.addConnection(3, 1, 8, 1);  // kick~ -> out~ L
    nodeGraph.addConnection(3, 1, 8, 2);  // kick~ -> out~ R
    nodeGraph.addConnection(5, 1, 8, 1);  // snare~ -> out~ L
    nodeGraph.addConnection(5, 1, 8, 2);  // snare~ -> out~ R
    nodeGraph.addConnection(7, 1, 8, 1);  // hihat~ -> out~ L
    nodeGraph.addConnection(7, 1, 8, 2);  // hihat~ -> out~ R

    nextNodeId = 9;
    arrangementTimelineComponent.refreshTimeline();
    canvasComponent.repaint();
    trackViewComponent.refreshTracks();
}

void WorkstationContainerComponent::toggleConsole()
{
    isConsoleVisible = !isConsoleVisible;
    consolePanel.setVisible(isConsoleVisible);
    if (isConsoleVisible) consolePanel.refreshLogs();
    resized();
    repaint();
}

void WorkstationContainerComponent::paint(juce::Graphics& g)
{
    g.fillAll(CarbonGoldLookAndFeel::carbonBg);

    // Top Header bar
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(0, 0, getWidth(), 60);

    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.3f));
    g.drawHorizontalLine(60, 0.0f, static_cast<float>(getWidth()));

    // Vertical Divider Line between canvas and inspector
    int consoleH = isConsoleVisible ? consoleHeight : 0;
    int availableH = getHeight() - 60 - consoleH;

    int dividerX = getWidth() - inspectorWidth;
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(dividerX, 60, inspectorWidth, availableH);

    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.4f));
    g.drawVerticalLine(dividerX, 60.0f, static_cast<float>(60 + availableH));
}

void WorkstationContainerComponent::mouseDown(const juce::MouseEvent& e)
{
    int dividerX = getWidth() - inspectorWidth;
    if (std::abs(e.position.x - dividerX) < 8 && e.position.y > 60)
    {
        isDraggingDivider = true;
    }
}

void WorkstationContainerComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingDivider)
    {
        int newW = getWidth() - static_cast<int>(e.position.x);
        inspectorWidth = std::clamp(newW, 160, 600);
        resized();
        repaint();
    }
}

void WorkstationContainerComponent::mouseMove(const juce::MouseEvent& e)
{
    int dividerX = getWidth() - inspectorWidth;
    if (std::abs(e.position.x - dividerX) < 8 && e.position.y > 60)
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void WorkstationContainerComponent::resized()
{
    // Flat Desktop Top Menu Bar
    titleLabel.setBounds(12, 16, 160, 28);

    int menuX = 180;
    int menuY = 16;
    int btnH = 28;

    fileMenuButton.setBounds(menuX, menuY, 44, btnH); menuX += 48;
    editMenuButton.setBounds(menuX, menuY, 44, btnH); menuX += 48;
    putMenuButton.setBounds(menuX, menuY, 44, btnH); menuX += 48;
    viewMenuButton.setBounds(menuX, menuY, 48, btnH); menuX += 52;
    workflowMenuButton.setBounds(menuX, menuY, 82, btnH); menuX += 86;
    helpMenuButton.setBounds(menuX, menuY, 48, btnH); menuX += 60;

    playButton.setBounds(menuX, menuY, 70, btnH); menuX += 75;
    stopButton.setBounds(menuX, menuY, 70, btnH); menuX += 80;

    trackViewButton.setBounds(menuX, menuY, 85, btnH); menuX += 89;
    canvasViewButton.setBounds(menuX, menuY, 105, btnH); menuX += 109;
    dualViewButton.setBounds(menuX, menuY, 105, btnH); menuX += 115;

    bpmLabel.setBounds(menuX, menuY + 4, 32, 20); menuX += 34;
    bpmSlider.setBounds(menuX, menuY, 95, btnH); menuX += 100;

    timeSigLabel.setBounds(menuX, menuY + 4, 25, 20); menuX += 27;
    timeSigCombo.setBounds(menuX, menuY, 60, btnH); menuX += 68;

    latencyLabel.setBounds(menuX, 18, 190, 18); menuX += 195;

    oscilloscopeComponent.setBounds(menuX, 6, std::max(50, getWidth() - menuX - 15), 48);

    // Resizable layout (TrackView, ModularCanvas, or DualViewSplit)
    int consoleH = isConsoleVisible ? consoleHeight : 0;
    int canvasW = getWidth() - inspectorWidth;
    int contentH = getHeight() - 60 - consoleH;

    if (currentViewMode == ViewMode::TrackView)
    {
        arrangementTimelineComponent.setVisible(true);
        trackViewComponent.setVisible(false);
        canvasComponent.setVisible(false);
        arrangementTimelineComponent.setBounds(0, 60, canvasW, contentH);
    }
    else if (currentViewMode == ViewMode::ModularCanvas)
    {
        arrangementTimelineComponent.setVisible(false);
        trackViewComponent.setVisible(false);
        canvasComponent.setVisible(true);
        canvasComponent.setBounds(0, 60, canvasW, contentH);
    }
    else // DualViewSplit
    {
        int topH = contentH / 2;
        int bottomH = contentH - topH;

        arrangementTimelineComponent.setVisible(true);
        trackViewComponent.setVisible(false);
        canvasComponent.setVisible(true);

        arrangementTimelineComponent.setBounds(0, 60, canvasW, topH);
        canvasComponent.setBounds(0, 60 + topH, canvasW, bottomH);
    }
    inspectorViewport.setBounds(canvasW + 2, 60, inspectorWidth - 2, contentH);
    int targetInspectorW = inspectorViewport.getViewWidth();
    nodeInspectorComponent.setBounds(0, 0, targetInspectorW, std::max(contentH, nodeInspectorComponent.getHeight()));
    nodeInspectorComponent.resized();

    if (isConsoleVisible)
    {
        consolePanel.setBounds(0, getHeight() - consoleH, getWidth(), consoleH);
    }
}

void WorkstationContainerComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    startupMuteBlocks = 20;
    nodeGraph.prepare(sampleRate, samplesPerBlockExpected);
}

void WorkstationContainerComponent::releaseResources()
{
}

void WorkstationContainerComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (bufferToFill.buffer != nullptr)
    {
        bufferToFill.buffer->clear();
    }

    if (!isPlaying || startupMuteBlocks > 0)
    {
        if (startupMuteBlocks > 0) --startupMuteBlocks;
        return;
    }

    nodeGraph.process(*bufferToFill.buffer, bufferToFill.numSamples);
    oscilloscopeComponent.pushBuffer(*bufferToFill.buffer);
}

void WorkstationContainerComponent::showAudioSettingsWindow()
{
    deviceManager.initialiseWithDefaultDevices(0, 2);

    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager,
        0, 2,  // min/max input channels
        0, 2,  // min/max output channels
        true,  // show stereo pair options
        true,  // show MIDI input options
        true,  // show sample rate options
        true   // show buffer size options
    );
    selector->setSize(500, 450);

    juce::DialogWindow::LaunchOptions opt;
    opt.dialogTitle = "Audio Settings (Interface, Sample Rate, Buffer Size)";
    opt.dialogBackgroundColour = CarbonGoldLookAndFeel::carbonBg;
    opt.content.setOwned(selector.release());
    opt.escapeKeyTriggersCloseButton = true;
    opt.useNativeTitleBar = true;
    opt.launchAsync();
}

void WorkstationContainerComponent::setupComposerTemplate()
{
    setupDefaultPatch();
}

void WorkstationContainerComponent::setupSoundDesignerTemplate()
{
    nodeGraph.clearGraph();

    auto oscNode = RelativisticNodeFactory::createNode(1, "osc~ saw");
    oscNode->xPos = 100; oscNode->yPos = 100;

    auto ladderNode = RelativisticNodeFactory::createNode(2, "ladder~ 3500 0.7");
    ladderNode->xPos = 280; ladderNode->yPos = 100;

    auto driveNode = RelativisticNodeFactory::createNode(3, "drive~ 3.5");
    driveNode->xPos = 460; driveNode->yPos = 100;

    auto outNode = RelativisticNodeFactory::createNode(4, "out~");
    outNode->xPos = 640; outNode->yPos = 100;

    nodeGraph.addNode(oscNode);
    nodeGraph.addNode(ladderNode);
    nodeGraph.addNode(driveNode);
    nodeGraph.addNode(outNode);

    nodeGraph.addConnection(1, 1, 2, 1); // osc -> ladder in~
    nodeGraph.addConnection(2, 0, 3, 0); // ladder -> drive in~
    nodeGraph.addConnection(3, 0, 4, 1); // drive -> out~ in1~ (Inlet 1, Audio L)
    nodeGraph.addConnection(3, 0, 4, 2); // drive -> out~ in2~ (Inlet 2, Audio R)

    nextNodeId = 5;
}

void WorkstationContainerComponent::setupFilmSciFiTemplate()
{
    nodeGraph.clearGraph();

    auto warpNode = RelativisticNodeFactory::createNode(1, "time.warp 2.5");
    warpNode->xPos = 60; warpNode->yPos = 80;

    auto pluckNode = RelativisticNodeFactory::createNode(2, "pluck~ 110");
    pluckNode->xPos = 240; pluckNode->yPos = 80;

    auto driveNode = RelativisticNodeFactory::createNode(3, "drive~ 4.0");
    driveNode->xPos = 420; driveNode->yPos = 80;

    auto outNode = RelativisticNodeFactory::createNode(4, "out~");
    outNode->xPos = 600; outNode->yPos = 80;

    nodeGraph.addNode(warpNode);
    nodeGraph.addNode(pluckNode);
    nodeGraph.addNode(driveNode);
    nodeGraph.addNode(outNode);

    nodeGraph.addConnection(1, 1, 2, 1); // time.warp timeOut -> pluck timeIn
    nodeGraph.addConnection(2, 0, 3, 0); // pluck out~ -> drive in~
    nodeGraph.addConnection(3, 0, 4, 1); // drive out~ -> out~ in1~ (Inlet 1, Audio L)
    nodeGraph.addConnection(3, 0, 4, 2); // drive out~ -> out~ in2~ (Inlet 2, Audio R)

    nextNodeId = 5;
}

void WorkstationContainerComponent::setupExperimentalistTemplate()
{
    nodeGraph.clearGraph();

    auto oscNode = RelativisticNodeFactory::createNode(1, "osc~ sqr");
    oscNode->xPos = 100; oscNode->yPos = 100;

    auto ladderNode = RelativisticNodeFactory::createNode(2, "ladder~ 1500 0.8");
    ladderNode->xPos = 280; ladderNode->yPos = 100;

    auto driveNode = RelativisticNodeFactory::createNode(3, "drive~ 2.0");
    driveNode->xPos = 460; driveNode->yPos = 100;

    auto outNode = RelativisticNodeFactory::createNode(4, "out~");
    outNode->xPos = 640; outNode->yPos = 100;

    nodeGraph.addNode(oscNode);
    nodeGraph.addNode(ladderNode);
    nodeGraph.addNode(driveNode);
    nodeGraph.addNode(outNode);

    nodeGraph.addConnection(1, 0, 2, 0); // osc -> ladder in~
    nodeGraph.addConnection(2, 0, 3, 0); // ladder -> drive in~
    nodeGraph.addConnection(3, 0, 4, 1); // drive -> out~ in1~ (Inlet 1, Audio L)
    nodeGraph.addConnection(3, 0, 4, 2); // drive -> out~ in2~ (Inlet 2, Audio R)

    // Feedback Loop: drive out~ (Outlet 0) back to ladder cutoff
    nodeGraph.addConnection(3, 0, 2, 2);

    nextNodeId = 5;
}

bool WorkstationContainerComponent::keyPressed(const juce::KeyPress& key)
{
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
    {
        if (key.getKeyCode() == 'S' || key.getKeyCode() == 's')
        {
            if (key.getModifiers().isShiftDown()) savePatchAs();
            else savePatch();
            return true;
        }
        else if (key.getKeyCode() == 'O' || key.getKeyCode() == 'o')
        {
            loadPatchFromFile(juce::File{});
            return true;
        }
        else if (key.getKeyCode() == 'N' || key.getKeyCode() == 'n')
        {
            newPatch();
            return true;
        }
        else if (key.getKeyCode() == 'K' || key.getKeyCode() == 'k')
        {
            toggleConsole();
            return true;
        }
    }
    return false;
}

void WorkstationContainerComponent::newPatch()
{
    currentPatchFile = juce::File{};
    setupDefaultPatch();
    titleLabel.setText("Time Dilation DAW 2 — Untitled.pdil", juce::dontSendNotification);
}

void WorkstationContainerComponent::savePatch()
{
    if (currentPatchFile.existsAsFile())
    {
        juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();

        // 1. Serialize Node Graph
        juce::String graphJsonStr = nodeGraph.serializeToJSON();
        auto parsedGraph = juce::JSON::parse(graphJsonStr);
        if (parsedGraph.isObject())
        {
            rootObj->setProperty("graph", parsedGraph);
        }

        // 2. Serialize Arrangement Timeline Message Events
        juce::Array<juce::var> eventsArr;
        for (const auto& ev : arrangementTimelineComponent.getMessageEvents())
        {
            juce::DynamicObject::Ptr eObj = new juce::DynamicObject();
            eObj->setProperty("eventId", ev.eventId);
            eObj->setProperty("targetNodeId", ev.targetNodeId);
            eObj->setProperty("trackIndex", ev.trackIndex);
            eObj->setProperty("timeSec", ev.timeSec);
            eObj->setProperty("messageText", ev.messageText);
            eventsArr.add(juce::var(eObj.get()));
        }
        rootObj->setProperty("timelineEvents", eventsArr);

        // 3. Serialize Master Transport & Loop Settings
        juce::DynamicObject::Ptr transportObj = new juce::DynamicObject();
        transportObj->setProperty("bpm", bpmSlider.getValue());
        transportObj->setProperty("timeSigId", timeSigCombo.getSelectedId());
        transportObj->setProperty("loopStartSec", arrangementTimelineComponent.getLoopStartSec());
        transportObj->setProperty("loopEndSec", arrangementTimelineComponent.getLoopEndSec());
        transportObj->setProperty("isLoopEnabled", arrangementTimelineComponent.isLoopActive());
        rootObj->setProperty("transport", juce::var(transportObj.get()));

        juce::String fullJson = juce::JSON::toString(juce::var(rootObj.get()), false);
        currentPatchFile.replaceWithText(fullJson);

        titleLabel.setText("Time Dilation DAW 2 — " + currentPatchFile.getFileName(), juce::dontSendNotification);
    }
    else
    {
        savePatchAs();
    }
}

void WorkstationContainerComponent::savePatchAs()
{
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Save Relativistic Patch As...",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Untitled.pdil"),
        "*.pdil");

    activeFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result != juce::File{})
            {
                currentPatchFile = result.withFileExtension(".pdil");
                savePatch();
            }
        });
}

void WorkstationContainerComponent::loadPatchFromFile(const juce::File& fileToLoad)
{
    if (fileToLoad.existsAsFile())
    {
        juce::String jsonStr = fileToLoad.loadFileAsString();
        auto parsed = juce::JSON::parse(jsonStr);
        if (parsed.isObject())
        {
            auto rootObj = parsed.getDynamicObject();
            if (rootObj)
            {
                // 1. Restore Node Graph
                if (rootObj->hasProperty("graph"))
                {
                    juce::String graphStr = juce::JSON::toString(rootObj->getProperty("graph"));
                    nodeGraph.deserializeFromJSON(graphStr.toStdString());
                }

                // 2. Restore Timeline Message Events
                arrangementTimelineComponent.clearMessageEvents();
                if (rootObj->hasProperty("timelineEvents"))
                {
                    auto evsVar = rootObj->getProperty("timelineEvents");
                    if (evsVar.isArray())
                    {
                        for (const auto& evVar : *evsVar.getArray())
                        {
                            if (auto eObj = evVar.getDynamicObject())
                            {
                                int targetNodeId = eObj->getProperty("targetNodeId");
                                int trackIdx = eObj->getProperty("trackIndex");
                                double timeSec = eObj->getProperty("timeSec");
                                juce::String msg = eObj->getProperty("messageText");
                                arrangementTimelineComponent.addMessageEvent(targetNodeId, trackIdx, timeSec, msg);
                            }
                        }
                    }
                }

                // 3. Restore Transport & Loop Settings
                if (rootObj->hasProperty("transport"))
                {
                    if (auto tObj = rootObj->getProperty("transport").getDynamicObject())
                    {
                        if (tObj->hasProperty("bpm")) bpmSlider.setValue(tObj->getProperty("bpm"));
                        if (tObj->hasProperty("timeSigId")) timeSigCombo.setSelectedId(tObj->getProperty("timeSigId"));
                        double lStart = tObj->hasProperty("loopStartSec") ? static_cast<double>(tObj->getProperty("loopStartSec")) : 0.0;
                        double lEnd = tObj->hasProperty("loopEndSec") ? static_cast<double>(tObj->getProperty("loopEndSec")) : 16.0;
                        bool lActive = tObj->hasProperty("isLoopEnabled") ? static_cast<bool>(tObj->getProperty("isLoopEnabled")) : true;
                        arrangementTimelineComponent.setLoopRange(lStart, lEnd, lActive);
                    }
                }

                currentPatchFile = fileToLoad;
                titleLabel.setText("Time Dilation DAW 2 — " + currentPatchFile.getFileName(), juce::dontSendNotification);
                canvasComponent.repaint();
                trackViewComponent.refreshTracks();
                arrangementTimelineComponent.refreshTimeline();
            }
        }
    }
    else
    {
        activeFileChooser = std::make_unique<juce::FileChooser>(
            "Open Relativistic Patch...",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.pdil");

        activeFileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result.existsAsFile())
                {
                    loadPatchFromFile(result);
                }
            });
    }
}

} // namespace TimeDilationDAW
