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
        m.addItem(2, "Open Patch (.pdil)...");
        m.addSeparator();
        m.addItem(3, "Save Patch (.pdil)");
        m.addItem(4, "Save As...");
        m.addSeparator();
        m.addItem(5, "Audio Settings (Interface, Sample Rate, Buffer Size)...");
        m.addSeparator();
        m.addItem(6, "Quit");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&fileMenuButton), [this](int result) {
            if (result == 1) setupDefaultPatch();
            else if (result == 5) showAudioSettingsWindow();
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
            trackViewComponent.refreshTracks();
        });
    };

    viewMenuButton.onClick = [this]() {
        juce::PopupMenu m;
        m.addItem(1, "Recenter Canvas (Cmd+0)");
        m.addItem(2, "Toggle Oscilloscope", true, true);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&viewMenuButton), [this](int result) {
            if (result == 1) { canvasComponent.recenterView(); }
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
        m.addItem(1, "Node Symbol Reference (35+ Symbols)");
        m.addItem(2, "Relativistic Time Math Guide");
        m.addSeparator();
        m.addItem(3, "About Time Dilation DAW 2");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&helpMenuButton), nullptr);
    };

    addAndMakeVisible(canvasComponent);
    addAndMakeVisible(trackViewComponent);
    addAndMakeVisible(arrangementTimelineComponent);
    addAndMakeVisible(nodeInspectorComponent);

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

    setupDefaultPatch();

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
    nodeGraph.clearGraph();

    auto lfoNode = RelativisticNodeFactory::createNode(1, "time.lfo 0.5 0.85");
    lfoNode->xPos = 50; lfoNode->yPos = 50;

    auto oscNode = RelativisticNodeFactory::createNode(2, "osc~ sin");
    oscNode->xPos = 220; oscNode->yPos = 120;

    auto ladderNode = RelativisticNodeFactory::createNode(3, "ladder~ 1500 0.6");
    ladderNode->xPos = 390; ladderNode->yPos = 120;

    auto driveNode = RelativisticNodeFactory::createNode(4, "drive~ 2.0");
    driveNode->xPos = 560; driveNode->yPos = 120;

    auto outNode = RelativisticNodeFactory::createNode(5, "out~");
    outNode->xPos = 730; outNode->yPos = 120;

    if (auto out = std::dynamic_pointer_cast<OutNode>(outNode))
    {
        out->onPlaybackStateChanged = [this](bool play) {
            isPlaying = play;
        };
    }

    nodeGraph.addNode(lfoNode);
    nodeGraph.addNode(oscNode);
    nodeGraph.addNode(ladderNode);
    nodeGraph.addNode(driveNode);
    nodeGraph.addNode(outNode);

    // Connect time.lfo~ timeOut (Outlet 1) -> osc~ timeIn (Inlet 1)
    nodeGraph.addConnection(1, 1, 2, 1);
    // Connect osc~ timeOut (Outlet 1) -> ladder~ timeIn (Inlet 1)
    nodeGraph.addConnection(2, 1, 3, 1);
    // Connect osc~ out~ (Outlet 2) -> ladder~ in~ (Inlet 2)
    nodeGraph.addConnection(2, 2, 3, 2);
    // Connect ladder~ timeOut (Outlet 1) -> drive~ timeIn (Inlet 1)
    nodeGraph.addConnection(3, 1, 4, 1);
    // Connect ladder~ out~ (Outlet 2) -> drive~ in~ (Inlet 2)
    nodeGraph.addConnection(3, 2, 4, 2);
    // Connect drive~ out~ (Outlet 2) -> out~ in~ (Inlet 1)
    nodeGraph.addConnection(4, 2, 5, 1);

    nextNodeId = 6;
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
    int dividerX = getWidth() - inspectorWidth;
    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.4f));
    g.drawVerticalLine(dividerX, 60.0f, static_cast<float>(getHeight()));
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
    int canvasW = getWidth() - inspectorWidth;
    int contentH = getHeight() - 60;

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
    nodeInspectorComponent.setBounds(canvasW + 2, 60, inspectorWidth - 2, contentH);
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
    nodeGraph.addConnection(3, 0, 4, 0); // drive -> out~ in1~

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
    nodeGraph.addConnection(3, 0, 4, 0); // drive out~ -> out~ in1~

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
    nodeGraph.addConnection(3, 0, 4, 0); // drive -> out~ in1~

    // Feedback Loop: drive out~ (Outlet 0) back to ladder cutoff
    nodeGraph.addConnection(3, 0, 2, 2);

    nextNodeId = 5;
}

} // namespace TimeDilationDAW
