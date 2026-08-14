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
        m.addItem(2, "Open Patch (.pdil) (Cmd+O)");
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

        juce::PopupMenu timeMenu;
        timeMenu.addItem(201, "time.curve~ (Hermite S-Curve Accelerator / Tape Stop)");
        timeMenu.addItem(202, "time.chaos~ (3D Lorenz / Rossler Chaotic Attractor)");
        timeMenu.addItem(203, "time.crossfade~ (Relativistic Spacetime Morpher)");
        timeMenu.addItem(204, "time.const~ (Static & Stepped Speed Dilation)");
        timeMenu.addItem(205, "time.scale~ (Time Multiplier & Polyrhythmic Divider)");
        timeMenu.addItem(206, "time.add~ (Time Bias & Groove Summer)");
        timeMenu.addItem(207, "time.quantize~ (Continuous-to-Stepped Grid Quantizer)");
        timeMenu.addItem(208, "time.split~ (Time Frame to Audio Signal Splitter)");
        timeMenu.addItem(209, "time.merge~ (Audio Signals to Time Frame Merger)");
        m.addSubMenu("Relativistic Time Sculptors", timeMenu);

        juce::PopupMenu soundMenu;
        soundMenu.addItem(301, "osc~ (Atomic Saw/Sin/Sqr/Tri Oscillator)");
        soundMenu.addItem(302, "noise~ (White / Paul Kellet Pink Noise)");
        soundMenu.addItem(303, "pluck~ (Karplus-Strong Physical Model)");
        soundMenu.addItem(304, "readsf~ (Streaming Audio File Playback)");
        soundMenu.addItem(305, "tabread~ (Table Array Player)");
        m.addSubMenu("Sound Generators & Players", soundMenu);

        juce::PopupMenu fxMenu;
        fxMenu.addItem(401, "ladder~ (Moog 4-Pole 24dB Resonant Filter)");
        fxMenu.addItem(402, "svf~ (State Variable Filter LP/HP/BP/Notch)");
        fxMenu.addItem(403, "delwrite~ (Relativistic Delay Line Writer)");
        fxMenu.addItem(404, "vd~ (Relativistic Doppler Variable Delay Reader)");
        fxMenu.addItem(405, "drive~ (Warm WaveShaper Tube Distortion)");
        fxMenu.addItem(406, "reverb~ (Feedback Delay Network Space Reverb)");
        m.addSubMenu("Filters & Effects", fxMenu);

        juce::PopupMenu ctrlMenu;
        ctrlMenu.addItem(501, "metro (Relativistic Proper-Time Clock)");
        ctrlMenu.addItem(502, "counter (Step Counter & Divider)");
        ctrlMenu.addItem(503, "random (Deterministic / Stochastic Generator)");
        ctrlMenu.addItem(504, "select (Value Matcher & Dispatcher)");
        ctrlMenu.addItem(505, "route (Prefix / Channel Router)");
        ctrlMenu.addItem(506, "t b b (Trigger Bangs in Right-to-Left Order)");
        ctrlMenu.addItem(507, "pipe (Proper-Time Timestamped Event Queue)");
        ctrlMenu.addItem(508, "timer (Relativistic Proper-Time Stopwatch)");
        ctrlMenu.addItem(509, "snapshot~ (Instantaneous Signal & Time Sampler)");
        ctrlMenu.addItem(510, "seq.euclid (Bjorklund Euclidean Rhythm Generator)");
        ctrlMenu.addItem(511, "seq.arp (Relativistic Chord Arpeggiator)");
        ctrlMenu.addItem(512, "seq.poly (Polyrhythmic Multi-Meter Sequencer)");
        ctrlMenu.addItem(513, "auto~ (Timeline Parameter Automation Reader)");
        m.addSubMenu("Control Logic & Sequencers", ctrlMenu);

        m.addSeparator();
        m.addItem(2, "osc~ Atomic Oscillator");
        m.addItem(3, "ladder~ Moog Ladder Filter");
        m.addItem(4, "delwrite~ + vd~ Tape Delay Line");
        m.addItem(5, "out~ Stereo Master Output");

        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&putMenuButton), [this](int result) {
            if (result == 1) { canvasComponent.spawnObjectEditorAt({ 200.0f, 200.0f }); }
            else if (result == 201) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.curve~ 1.0 500"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 202) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.chaos~ 0.5 lorenz"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 203) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.crossfade~ 0.5"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 204) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.const~ 1.0"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 205) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.scale~ 2.0"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 206) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.add~ 0.2"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 207) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.quantize~ 125"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 208) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.split~"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 209) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "time.merge~"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 301 || result == 2) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "osc~ saw"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 302) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "noise~ white"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 303) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "pluck~ 220"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 304) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "readsf~ 2"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 305) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "tabread~ array1"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 401 || result == 3) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "ladder~ 2200 0.65"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 402) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "svf~ 1500 0.707"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 403 || result == 4) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "delwrite~ del1 1000"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 404) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "vd~ del1 150"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 405) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "drive~ 2.0"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 406) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "reverb~ 0.75 0.4"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 501) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "metro 125 1"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 502) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "counter 0 15 1"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 503) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "random 100"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 504) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "select 0 4 8 12 15"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 505) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "route 1 2 3"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 506) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "t b b"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 507) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "pipe 150"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 508) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "timer"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 509) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "snapshot~"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 510) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "seq.euclid 5 16"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 511) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "seq.arp up 2 0.125"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 512) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "seq.poly"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 513) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "auto~ 0.5"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
            else if (result == 5) { auto n = RelativisticNodeFactory::createNode(nextNodeId++, "out~"); n->xPos = 200; n->yPos = 150; canvasComponent.getCurrentGraph().addNode(n); canvasComponent.repaint(); }
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
        m.addItem(1, "Composer / Musician Mode (Synth Lead + Pluck String)");
        m.addItem(2, "Sound Designer Mode (Moog Ladder Filter + WaveShaper)");
        m.addItem(3, "Film & Game Sci-Fi Mode (Relativistic Doppler Wormhole)");
        m.addItem(4, "Experimentalist Mode (Tarjan Feedback Chaos Loop)");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&workflowMenuButton), [this](int result) {
            if (result == 1) setupComposerTemplate();
            else if (result == 2) setupSoundDesignerTemplate();
            else if (result == 3) setupFilmSciFiTemplate();
            else if (result == 4) setupExperimentalistTemplate();
            trackViewComponent.refreshTracks();
            canvasComponent.repaint();
        });
    };

    helpMenuButton.onClick = [this]() {
        juce::PopupMenu m;

        juce::PopupMenu examplesMenu;
        examplesMenu.addItem(101, "01: Full Workstation Ensemble (Synth + Drums + Space FX)");
        examplesMenu.addItem(102, "02: Analog Drum Machine & Groove (Kick, Snare, Hi-Hat)");
        examplesMenu.addItem(103, "03: Deterministic Tape Stop & Wobble Machine (Metro -> Select -> Time.Curve)");
        examplesMenu.addItem(104, "04: Relativistic Delay & Pipe Synth (Doppler Delay + Proper Time Pipe)");
        examplesMenu.addItem(105, "05: Multi-Branch Time Morph & Chaos Rig (Lorenz RK4 + Hermite Morph)");
        examplesMenu.addItem(106, "06: Relativistic Euclidean & Timeline Arrangement Rig (Euclid 5/16 + Arp + Automation)");
        m.addSubMenu("Examples & Presets", examplesMenu);

        m.addSeparator();
        m.addItem(1, "Node Symbol Reference (35+ Symbols)");
        m.addItem(2, "Relativistic Time Math Guide");
        m.addSeparator();
        m.addItem(3, "About Time Dilation DAW 2");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&helpMenuButton), [this](int result) {
            if (result == 101) loadExampleFullEnsemble();
            else if (result == 102) loadExampleDrumGroove();
            else if (result == 103) loadExampleTapeStopWobble();
            else if (result == 104) loadExampleDelayPipeSynth();
            else if (result == 105) loadExampleChaosMorph();
            else if (result == 106) loadExampleEuclideanArrangement();
            else if (result == 1) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Node Symbol Reference",
                    "Time Objects:\n- time.curve~, time.chaos~, time.crossfade~, time.const~, time.scale~, time.add~, time.quantize~, time.split~, time.merge~\n\n"
                    "Sequencer & Control:\n- seq.euclid, seq.arp, seq.poly, auto~, metro, counter, random, select, route, t b b, pipe, timer, snapshot~\n\n"
                    "Audio Sources & FX:\n- osc~, noise~, pluck~, readsf~, tabread~, ladder~, svf~, delwrite~, vd~, drive~, reverb~, out~");
            }
            else if (result == 2) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Relativistic Time Math Guide",
                    "Proper Time: \u03c4 = \u222b \u03b3(t) dt\n"
                    "Bjorklund Euclidean: E(k, n) = maximally even pulse distribution\n"
                    "Doppler Delay Read: d\u03c4_read / dt = 1 - (1/\u03b3) \u00b7 (v/c)\n"
                    "C2 Hermite Smoothstep: h(u) = 3u\u00b2 - 2u\u00b3");
            }
            else if (result == 3) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "About Time Dilation DAW 2",
                    "Time Dilation DAW 2\nRelativistic Non-Linear Music Production Workstation\nEngine: 96 kHz Proper-Time Relativistic Vector Engine\nDesign: Carbon & Gold Pro Console");
            }
        });
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
        int consoleH = isConsoleVisible ? consoleHeight : 0;
        int contentH = getHeight() - 60 - consoleH;
        int targetW = std::max(180, inspectorWidth - 10);
        nodeInspectorComponent.setBounds(0, 0, targetW, std::max(contentH, nodeInspectorComponent.getHeight()));
        nodeInspectorComponent.resized();
        nodeInspectorComponent.repaint();
        inspectorViewport.repaint();
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

void WorkstationContainerComponent::loadExampleTapeStopWobble()
{
    nodeGraph.clearGraph();

    // Row 1: Spacetime Master & Clock Control
    auto timeCurve  = RelativisticNodeFactory::createNode(1, "time.curve~ 1.0 400");
    timeCurve->xPos = 60; timeCurve->yPos = 40;

    auto metroClock = RelativisticNodeFactory::createNode(2, "metro 125 1");
    metroClock->xPos = 240; metroClock->yPos = 40;

    auto trig       = RelativisticNodeFactory::createNode(3, "t b b");
    trig->xPos = 420; trig->yPos = 40;

    auto counter    = RelativisticNodeFactory::createNode(4, "counter 0 15 1");
    counter->xPos = 580; counter->yPos = 40;

    auto rnd        = RelativisticNodeFactory::createNode(5, "random 100");
    rnd->xPos = 740; rnd->yPos = 40;

    // Row 2: State Machine & Sound Generators
    auto sel        = RelativisticNodeFactory::createNode(6, "select 0 4 8 12 15");
    sel->xPos = 60; sel->yPos = 200;

    auto seq        = RelativisticNodeFactory::createNode(7, "seq 36 36 48 51 53 55 58 60 48 51 63 60 36 48 55 58");
    seq->xPos = 240; seq->yPos = 200;

    auto mtof       = RelativisticNodeFactory::createNode(8, "mtof");
    mtof->xPos = 420; mtof->yPos = 200;

    auto osc        = RelativisticNodeFactory::createNode(9, "osc~ saw");
    osc->xPos = 580; osc->yPos = 200;
    osc->setOutputVolume(0.85f);

    auto noise      = RelativisticNodeFactory::createNode(10, "noise~ white");
    noise->xPos = 740; noise->yPos = 200;
    noise->setOutputVolume(0.35f);

    // Row 3: Resonant Filter, Doppler Delay & Stereo Master
    auto filter     = RelativisticNodeFactory::createNode(11, "ladder~ 2200 0.65");
    filter->xPos = 60; filter->yPos = 360;

    auto delwrite   = RelativisticNodeFactory::createNode(12, "delwrite~ tape_deck 2000");
    delwrite->xPos = 240; delwrite->yPos = 360;

    auto tapL       = RelativisticNodeFactory::createNode(13, "vd~ tape_deck 140");
    tapL->xPos = 420; tapL->yPos = 360;

    auto tapR       = RelativisticNodeFactory::createNode(14, "vd~ tape_deck 280");
    tapR->xPos = 580; tapR->yPos = 360;

    auto drive      = RelativisticNodeFactory::createNode(15, "drive~ 1.6");
    drive->xPos = 740; drive->yPos = 360;

    auto out        = RelativisticNodeFactory::createNode(16, "out~ master");
    out->xPos = 920; out->yPos = 360;

    nodeGraph.addNode(timeCurve);
    nodeGraph.addNode(metroClock);
    nodeGraph.addNode(trig);
    nodeGraph.addNode(counter);
    nodeGraph.addNode(rnd);
    nodeGraph.addNode(sel);
    nodeGraph.addNode(seq);
    nodeGraph.addNode(mtof);
    nodeGraph.addNode(osc);
    nodeGraph.addNode(noise);
    nodeGraph.addNode(filter);
    nodeGraph.addNode(delwrite);
    nodeGraph.addNode(tapL);
    nodeGraph.addNode(tapR);
    nodeGraph.addNode(drive);
    nodeGraph.addNode(out);

    // Relativistic Time Distribution
    nodeGraph.addConnection(1, 1, 2, 1);  // timeCurve -> metro
    nodeGraph.addConnection(1, 1, 9, 1);  // timeCurve -> osc
    nodeGraph.addConnection(1, 1, 11, 1); // timeCurve -> filter
    nodeGraph.addConnection(1, 1, 12, 2); // timeCurve -> delwrite
    nodeGraph.addConnection(1, 1, 13, 2); // timeCurve -> tapL
    nodeGraph.addConnection(1, 1, 14, 2); // timeCurve -> tapR

    // Deterministic Control Flow (metro -> t b b -> counter + random -> select)
    nodeGraph.addConnection(2, 0, 3, 0);  // metro -> trig
    nodeGraph.addConnection(3, 0, 4, 0);  // trig -> counter
    nodeGraph.addConnection(3, 1, 5, 0);  // trig -> random
    nodeGraph.addConnection(4, 0, 6, 0);  // counter -> select
    nodeGraph.addConnection(4, 0, 7, 0);  // counter -> seq
    nodeGraph.addConnection(7, 1, 8, 1);  // seq -> mtof
    nodeGraph.addConnection(8, 1, 9, 2);  // mtof -> osc freq

    // Wire select state machine triggers
    nodeGraph.addConnection(6, 0, 1, 0); // beat 0 -> resume / normal 1.0
    nodeGraph.addConnection(6, 1, 1, 0); // beat 4 -> flutter speed up
    nodeGraph.addConnection(6, 2, 1, 0); // beat 8 -> tape sag
    nodeGraph.addConnection(6, 3, 1, 0); // beat 12 -> tape stop brake
    nodeGraph.addConnection(6, 4, 1, 0); // beat 15 -> spin up start

    // Audio & Tape Loop Connections
    nodeGraph.addConnection(9, 2, 11, 2);  // osc saw -> ladder in
    nodeGraph.addConnection(10, 2, 11, 2); // noise -> ladder in
    nodeGraph.addConnection(11, 2, 12, 1); // ladder -> delwrite
    nodeGraph.addConnection(11, 2, 15, 2); // ladder -> drive
    nodeGraph.addConnection(15, 2, 16, 1); // drive -> out L
    nodeGraph.addConnection(13, 2, 16, 1); // tapL -> out L
    nodeGraph.addConnection(14, 2, 16, 2); // tapR -> out R

    nextNodeId = 17;
    titleLabel.setText("Time Dilation DAW 2 — Deterministic Tape Stop & Wobble Machine", juce::dontSendNotification);
    arrangementTimelineComponent.refreshTimeline();
    canvasComponent.repaint();
    trackViewComponent.refreshTracks();
}

void WorkstationContainerComponent::loadExampleDelayPipeSynth()
{
    nodeGraph.clearGraph();

    auto lfoTime   = RelativisticNodeFactory::createNode(1, "time.lfo 0.3 0.6");
    lfoTime->xPos = 60; lfoTime->yPos = 40;

    auto metro     = RelativisticNodeFactory::createNode(2, "metro 140 1");
    metro->xPos = 240; metro->yPos = 40;

    auto seq       = RelativisticNodeFactory::createNode(3, "seq 48 51 55 58 60 63 67 70");
    seq->xPos = 420; seq->yPos = 40;

    auto mtof      = RelativisticNodeFactory::createNode(4, "mtof");
    mtof->xPos = 580; mtof->yPos = 40;

    auto pipePitch = RelativisticNodeFactory::createNode(5, "pipe 180");
    pipePitch->xPos = 740; pipePitch->yPos = 40;

    auto osc       = RelativisticNodeFactory::createNode(6, "osc~ saw");
    osc->xPos = 60; osc->yPos = 220;
    osc->setOutputVolume(0.85f);

    auto filter    = RelativisticNodeFactory::createNode(7, "ladder~ 2400 0.6");
    filter->xPos = 240; filter->yPos = 220;

    auto delwrite  = RelativisticNodeFactory::createNode(8, "delwrite~ space_echo 2000");
    delwrite->xPos = 420; delwrite->yPos = 220;

    auto tapL      = RelativisticNodeFactory::createNode(9, "vd~ space_echo 160");
    tapL->xPos = 580; tapL->yPos = 220;

    auto tapR      = RelativisticNodeFactory::createNode(10, "vd~ space_echo 320");
    tapR->xPos = 740; tapR->yPos = 220;

    auto out       = RelativisticNodeFactory::createNode(11, "out~ master");
    out->xPos = 920; out->yPos = 220;

    nodeGraph.addNode(lfoTime);
    nodeGraph.addNode(metro);
    nodeGraph.addNode(seq);
    nodeGraph.addNode(mtof);
    nodeGraph.addNode(pipePitch);
    nodeGraph.addNode(osc);
    nodeGraph.addNode(filter);
    nodeGraph.addNode(delwrite);
    nodeGraph.addNode(tapL);
    nodeGraph.addNode(tapR);
    nodeGraph.addNode(out);

    nodeGraph.addConnection(1, 1, 2, 1);  // lfoTime -> metro
    nodeGraph.addConnection(1, 1, 5, 1);  // lfoTime -> pipe
    nodeGraph.addConnection(1, 1, 6, 1);  // lfoTime -> osc
    nodeGraph.addConnection(1, 1, 7, 1);  // lfoTime -> filter
    nodeGraph.addConnection(1, 1, 8, 2);  // lfoTime -> delwrite
    nodeGraph.addConnection(1, 1, 9, 2);  // lfoTime -> tapL
    nodeGraph.addConnection(1, 1, 10, 2); // lfoTime -> tapR

    nodeGraph.addConnection(2, 0, 3, 0);  // metro -> seq
    nodeGraph.addConnection(3, 1, 4, 1);  // seq -> mtof
    nodeGraph.addConnection(4, 1, 5, 0);  // mtof -> pipe
    nodeGraph.addConnection(5, 0, 6, 2);  // pipe -> osc freq

    nodeGraph.addConnection(6, 2, 7, 2);  // osc -> filter
    nodeGraph.addConnection(7, 2, 8, 1);  // filter -> delwrite
    nodeGraph.addConnection(7, 2, 11, 1); // filter -> out L
    nodeGraph.addConnection(7, 2, 11, 2); // filter -> out R
    nodeGraph.addConnection(9, 2, 11, 1); // tapL -> out L
    nodeGraph.addConnection(10, 2, 11, 2); // tapR -> out R

    nextNodeId = 12;
    titleLabel.setText("Time Dilation DAW 2 — Relativistic Delay & Pipe Synth", juce::dontSendNotification);
    arrangementTimelineComponent.refreshTimeline();
    canvasComponent.repaint();
    trackViewComponent.refreshTracks();
}

void WorkstationContainerComponent::loadExampleChaosMorph()
{
    nodeGraph.clearGraph();

    auto chaosTime  = RelativisticNodeFactory::createNode(1, "time.chaos~ 0.35 lorenz");
    chaosTime->xPos = 60; chaosTime->yPos = 40;

    auto curveTime  = RelativisticNodeFactory::createNode(2, "time.curve~ 1.5 2000");
    curveTime->xPos = 240; curveTime->yPos = 40;

    auto xfadeTime  = RelativisticNodeFactory::createNode(3, "time.crossfade~ 0.5");
    xfadeTime->xPos = 420; xfadeTime->yPos = 40;

    auto lfoMixMod  = RelativisticNodeFactory::createNode(4, "osc~ sin");
    lfoMixMod->xPos = 600; lfoMixMod->yPos = 40;

    auto metroClock = RelativisticNodeFactory::createNode(5, "metro 160 1");
    metroClock->xPos = 60; metroClock->yPos = 200;

    auto counter    = RelativisticNodeFactory::createNode(6, "counter 0 7 1");
    counter->xPos = 240; counter->yPos = 200;

    auto seqPitch   = RelativisticNodeFactory::createNode(7, "seq 48 51 55 58 60 63 67 70");
    seqPitch->xPos = 420; seqPitch->yPos = 200;

    auto mtofNode   = RelativisticNodeFactory::createNode(8, "mtof");
    mtofNode->xPos = 600; mtofNode->yPos = 200;

    auto oscNode    = RelativisticNodeFactory::createNode(9, "osc~ saw");
    oscNode->xPos = 60; oscNode->yPos = 360;

    auto filterNode = RelativisticNodeFactory::createNode(10, "ladder~ 1800 0.7");
    filterNode->xPos = 240; filterNode->yPos = 360;

    auto delwrite   = RelativisticNodeFactory::createNode(11, "delwrite~ chaos_tape 2500");
    delwrite->xPos = 420; delwrite->yPos = 360;

    auto tapL       = RelativisticNodeFactory::createNode(12, "vd~ chaos_tape 175");
    tapL->xPos = 600; tapL->yPos = 360;

    auto tapR       = RelativisticNodeFactory::createNode(13, "vd~ chaos_tape 350");
    tapR->xPos = 760; tapR->yPos = 360;

    auto driveNode  = RelativisticNodeFactory::createNode(14, "drive~ 1.8");
    driveNode->xPos = 920; driveNode->yPos = 360;

    auto outNode    = RelativisticNodeFactory::createNode(15, "out~ master");
    outNode->xPos = 1080; outNode->yPos = 360;

    nodeGraph.addNode(chaosTime);
    nodeGraph.addNode(curveTime);
    nodeGraph.addNode(xfadeTime);
    nodeGraph.addNode(lfoMixMod);
    nodeGraph.addNode(metroClock);
    nodeGraph.addNode(counter);
    nodeGraph.addNode(seqPitch);
    nodeGraph.addNode(mtofNode);
    nodeGraph.addNode(oscNode);
    nodeGraph.addNode(filterNode);
    nodeGraph.addNode(delwrite);
    nodeGraph.addNode(tapL);
    nodeGraph.addNode(tapR);
    nodeGraph.addNode(driveNode);
    nodeGraph.addNode(outNode);

    nodeGraph.addConnection(1, 1, 3, 1);  // chaos timeOut -> xfade timeIn1
    nodeGraph.addConnection(2, 1, 3, 2);  // curve timeOut -> xfade timeIn2
    nodeGraph.addConnection(4, 2, 3, 3);  // LFO out~ -> xfade mixMod~

    nodeGraph.addConnection(3, 1, 5, 1);  // xfade -> metro
    nodeGraph.addConnection(3, 1, 9, 1);  // xfade -> osc
    nodeGraph.addConnection(3, 1, 10, 1); // xfade -> ladder
    nodeGraph.addConnection(3, 1, 11, 2); // xfade -> delwrite
    nodeGraph.addConnection(3, 1, 12, 2); // xfade -> tapL
    nodeGraph.addConnection(3, 1, 13, 2); // xfade -> tapR

    nodeGraph.addConnection(5, 0, 6, 0);  // metro -> counter
    nodeGraph.addConnection(6, 0, 7, 0);  // counter -> seq
    nodeGraph.addConnection(7, 1, 8, 1);  // seq -> mtof
    nodeGraph.addConnection(8, 1, 9, 2);  // mtof -> osc freq
    nodeGraph.addConnection(9, 2, 10, 2); // osc -> ladder
    nodeGraph.addConnection(10, 2, 11, 1); // ladder -> delwrite
    nodeGraph.addConnection(10, 2, 14, 2); // ladder -> drive
    nodeGraph.addConnection(14, 2, 15, 1); // drive -> out L
    nodeGraph.addConnection(12, 2, 15, 1); // tapL -> out L
    nodeGraph.addConnection(13, 2, 15, 2); // tapR -> out R

    nextNodeId = 16;
    titleLabel.setText("Time Dilation DAW 2 — Multi-Branch Time Morph & Chaos Rig", juce::dontSendNotification);
    arrangementTimelineComponent.refreshTimeline();
    canvasComponent.repaint();
    trackViewComponent.refreshTracks();
}

void WorkstationContainerComponent::loadExampleEuclideanArrangement()
{
    nodeGraph.clearGraph();

    // 1. Spacetime Clock & Modulation Master
    auto timeCurve  = RelativisticNodeFactory::createNode(1, "time.curve~ 1.0 400");
    timeCurve->xPos = 60; timeCurve->yPos = 40;

    auto timeLfo    = RelativisticNodeFactory::createNode(2, "osc~ sin");
    timeLfo->xPos = 240; timeLfo->yPos = 40;

    // 2. Euclidean Rhythm Generator & Drum Synthesizers
    auto euclidKick = RelativisticNodeFactory::createNode(3, "seq.euclid 4 16 0");
    euclidKick->setLabel("euclid.kick");
    euclidKick->xPos = 420; euclidKick->yPos = 40;

    auto kickSynth  = RelativisticNodeFactory::createNode(4, "kick~ 55 0.35");
    kickSynth->setOutputVolume(0.95f);
    kickSynth->xPos = 600; kickSynth->yPos = 40;

    auto euclidHat  = RelativisticNodeFactory::createNode(5, "seq.euclid 7 16 2");
    euclidHat->setLabel("euclid.hihat");
    euclidHat->xPos = 780; euclidHat->yPos = 40;

    auto hatSynth   = RelativisticNodeFactory::createNode(6, "hihat~ 0.08");
    hatSynth->setOutputVolume(0.70f);
    hatSynth->xPos = 960; hatSynth->yPos = 40;

    // 3. Melodic Arpeggiator & Moog Ladder Synth Chain
    auto arpNode    = RelativisticNodeFactory::createNode(7, "seq.arp updown 2 0.125");
    arpNode->xPos = 60; arpNode->yPos = 220;

    auto oscLead    = RelativisticNodeFactory::createNode(8, "osc~ saw");
    oscLead->setOutputVolume(0.85f);
    oscLead->xPos = 240; oscLead->yPos = 220;

    auto ladderFilt = RelativisticNodeFactory::createNode(9, "ladder~ 2400 0.65");
    ladderFilt->xPos = 420; ladderFilt->yPos = 220;

    auto autoCutoff = RelativisticNodeFactory::createNode(10, "auto~ 0.5");
    autoCutoff->xPos = 600; autoCutoff->yPos = 220;

    // 4. Stereo Tape Doppler Echo & Master Output
    auto delwrite   = RelativisticNodeFactory::createNode(11, "delwrite~ euclid_echo 2000");
    delwrite->xPos = 780; delwrite->yPos = 220;

    auto tapL       = RelativisticNodeFactory::createNode(12, "vd~ euclid_echo 160");
    tapL->xPos = 60; tapL->yPos = 380;

    auto tapR       = RelativisticNodeFactory::createNode(13, "vd~ euclid_echo 320");
    tapR->xPos = 240; tapR->yPos = 380;

    auto driveFx    = RelativisticNodeFactory::createNode(14, "drive~ 1.8");
    driveFx->xPos = 420; driveFx->yPos = 380;

    auto outMaster  = RelativisticNodeFactory::createNode(15, "out~ master");
    outMaster->xPos = 640; outMaster->yPos = 380;

    nodeGraph.addNode(timeCurve);
    nodeGraph.addNode(timeLfo);
    nodeGraph.addNode(euclidKick);
    nodeGraph.addNode(kickSynth);
    nodeGraph.addNode(euclidHat);
    nodeGraph.addNode(hatSynth);
    nodeGraph.addNode(arpNode);
    nodeGraph.addNode(oscLead);
    nodeGraph.addNode(ladderFilt);
    nodeGraph.addNode(autoCutoff);
    nodeGraph.addNode(delwrite);
    nodeGraph.addNode(tapL);
    nodeGraph.addNode(tapR);
    nodeGraph.addNode(driveFx);
    nodeGraph.addNode(outMaster);

    // Relativistic Clock Connections
    nodeGraph.addConnection(1, 1, 3, 1);  // timeCurve -> euclidKick
    nodeGraph.addConnection(1, 1, 5, 1);  // timeCurve -> euclidHat
    nodeGraph.addConnection(1, 1, 7, 1);  // timeCurve -> arpNode
    nodeGraph.addConnection(1, 1, 8, 1);  // timeCurve -> oscLead
    nodeGraph.addConnection(1, 1, 9, 1);  // timeCurve -> ladderFilt
    nodeGraph.addConnection(1, 1, 11, 2); // timeCurve -> delwrite
    nodeGraph.addConnection(1, 1, 12, 2); // timeCurve -> tapL
    nodeGraph.addConnection(1, 1, 13, 2); // timeCurve -> tapR

    // Rhythm Triggers
    nodeGraph.addConnection(3, 1, 4, 1);  // euclidKick gate -> kick~ trig
    nodeGraph.addConnection(5, 1, 6, 1);  // euclidHat gate -> hihat~ trig

    // Melodic Arp -> Synth
    nodeGraph.addConnection(7, 2, 8, 2);  // arp freq~ -> osc freq~
    nodeGraph.addConnection(8, 2, 9, 2);  // osc -> ladder
    nodeGraph.addConnection(10, 1, 9, 3); // auto~ -> ladder cutoff mod

    // Effects & Stereo Master
    nodeGraph.addConnection(9, 2, 11, 1);  // ladder -> delwrite
    nodeGraph.addConnection(9, 2, 14, 2);  // ladder -> drive
    nodeGraph.addConnection(14, 2, 15, 1); // drive -> out L
    nodeGraph.addConnection(14, 2, 15, 2); // drive -> out R
    nodeGraph.addConnection(4, 1, 15, 1);  // kick -> out L
    nodeGraph.addConnection(4, 1, 15, 2);  // kick -> out R
    nodeGraph.addConnection(6, 1, 15, 1);  // hihat -> out L
    nodeGraph.addConnection(6, 1, 15, 2);  // hihat -> out R
    nodeGraph.addConnection(12, 2, 15, 1); // tapL -> out L
    nodeGraph.addConnection(13, 2, 15, 2); // tapR -> out R

    nextNodeId = 16;
    titleLabel.setText("Time Dilation DAW 2 — Relativistic Euclidean & Timeline Arrangement Rig", juce::dontSendNotification);
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
    nodeGraph.clearGraph();
    arrangementTimelineComponent.clearMessageEvents();
    currentPatchFile = juce::File{};

    // Add default Master Out~ node ready for immediate patching
    auto outNode = RelativisticNodeFactory::createNode(1, "out~ master");
    outNode->xPos = 450.0f;
    outNode->yPos = 180.0f;
    outNode->setOutputVolume(1.0f);

    if (auto out = std::dynamic_pointer_cast<OutNode>(outNode))
    {
        out->onPlaybackStateChanged = [this](bool play) {
            isPlaying = play;
        };
    }

    nodeGraph.addNode(outNode);
    nextNodeId = 2;

    titleLabel.setText("Time Dilation DAW 2 — Untitled.pdil", juce::dontSendNotification);
    canvasComponent.repaint();
    trackViewComponent.refreshTracks();
    arrangementTimelineComponent.refreshTimeline();

    ConsoleLogger::getInstance().log("Created new relativistic patch (Untitled.pdil)", "patch", LogLevel::System);
}

void WorkstationContainerComponent::savePatch()
{
    if (currentPatchFile.existsAsFile() || currentPatchFile != juce::File{})
    {
        juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();

        // 1. Serialize Node Graph (Nodes, Cables, Coordinates, Volume & Scope modes, Feedback Safety)
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
        transportObj->setProperty("viewMode", static_cast<int>(currentViewMode));
        rootObj->setProperty("transport", juce::var(transportObj.get()));

        juce::String fullJson = juce::JSON::toString(juce::var(rootObj.get()), true);
        if (currentPatchFile.replaceWithText(fullJson))
        {
            titleLabel.setText("Time Dilation DAW 2 — " + currentPatchFile.getFileName(), juce::dontSendNotification);
            ConsoleLogger::getInstance().log("Patch saved successfully: " + currentPatchFile.getFullPathName().toStdString(), "patch", LogLevel::System);
        }
        else
        {
            ConsoleLogger::getInstance().log("Failed to write patch file to: " + currentPatchFile.getFullPathName().toStdString(), "patch", LogLevel::Error);
        }
    }
    else
    {
        savePatchAs();
    }
}

void WorkstationContainerComponent::savePatchAs()
{
    juce::File defaultDir = currentPatchFile.existsAsFile() ? currentPatchFile.getParentDirectory()
                                                            : juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    juce::String defaultName = currentPatchFile.existsAsFile() ? currentPatchFile.getFileName() : "Untitled.pdil";

    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Save Relativistic Patch As...",
        defaultDir.getChildFile(defaultName),
        "*.pdil;*.json");

    activeFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
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

                // Update nextNodeId beyond any existing node
                nextNodeId = 1;
                for (const auto& n : nodeGraph.getNodes())
                {
                    nextNodeId = std::max(nextNodeId, n->getId() + 1);
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

                        if (tObj->hasProperty("viewMode"))
                        {
                            currentViewMode = static_cast<ViewMode>(static_cast<int>(tObj->getProperty("viewMode")));
                        }
                    }
                }

                currentPatchFile = fileToLoad;
                titleLabel.setText("Time Dilation DAW 2 — " + currentPatchFile.getFileName(), juce::dontSendNotification);
                canvasComponent.repaint();
                trackViewComponent.refreshTracks();
                arrangementTimelineComponent.refreshTimeline();
                resized();

                ConsoleLogger::getInstance().log("Loaded patch: " + currentPatchFile.getFullPathName().toStdString() + " (" + std::to_string(nodeGraph.getNodes().size()) + " nodes)", "patch", LogLevel::System);
            }
        }
        else
        {
            ConsoleLogger::getInstance().log("Invalid patch format in: " + fileToLoad.getFullPathName().toStdString(), "patch", LogLevel::Error);
        }
    }
    else
    {
        juce::File initialDir = currentPatchFile.existsAsFile() ? currentPatchFile.getParentDirectory()
                                                                : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

        activeFileChooser = std::make_unique<juce::FileChooser>(
            "Open Relativistic Patch (.pdil)...",
            initialDir,
            "*.pdil;*.json");

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
