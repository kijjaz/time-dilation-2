#include "NodeInspectorComponent.h"
#include "CarbonGoldLookAndFeel.h"

namespace TimeDilationDAW
{

NodeInspectorComponent::NodeInspectorComponent()
{
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(titleLabel);

    nodeTypeLabel.setFont(juce::Font(12.0f, juce::Font::italic));
    nodeTypeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(nodeTypeLabel);

    // Geometry Controls
    posXSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    posXSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(posXSlider);
    posXLabel.setFont(11.0f);
    posXLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(posXLabel);

    posYSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    posYSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(posYSlider);
    posYLabel.setFont(11.0f);
    posYLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(posYLabel);

    widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    widthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(widthSlider);
    widthLabel.setFont(11.0f);
    widthLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(widthLabel);

    heightSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    heightSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(heightSlider);
    heightLabel.setFont(11.0f);
    heightLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(heightLabel);

    // Realtime Scope Controls
    scopeVisibleToggle.setColour(juce::ToggleButton::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(scopeVisibleToggle);

    scopeTypeCombo.addItem("Audio Waveform", 1);
    scopeTypeCombo.addItem("Time Speed (γ)", 2);
    scopeTypeCombo.addItem("Time Offset (τ)", 3);
    scopeTypeCombo.addItem("Time Elasticity (C)", 4);
    scopeTypeCombo.addItem("Multi-Time (γ + τ)", 5);
    addAndMakeVisible(scopeTypeCombo);
    scopeTypeLabel.setFont(11.0f);
    scopeTypeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(scopeTypeLabel);

    scopeEngineCombo.addItem("2D Waveform", 1);
    scopeEngineCombo.addItem("XY Lissajous", 2);
    scopeEngineCombo.addItem("3D Projection", 3);
    addAndMakeVisible(scopeEngineCombo);
    scopeEngineLabel.setFont(11.0f);
    scopeEngineLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(scopeEngineLabel);

    // Per-Node Output Gain Staging Volume Slider
    volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(volSlider);
    volLabel.setFont(12.0f);
    volLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(volLabel);

    // Param 1
    paramSlider1.setSliderStyle(juce::Slider::LinearHorizontal);
    paramSlider1.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(paramSlider1);
    paramLabel1.setFont(12.0f);
    paramLabel1.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(paramLabel1);

    // Param 2
    paramSlider2.setSliderStyle(juce::Slider::LinearHorizontal);
    paramSlider2.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(paramSlider2);
    paramLabel2.setFont(12.0f);
    paramLabel2.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(paramLabel2);

    // Option Dropdown
    addAndMakeVisible(optionSelector);
    optionLabel.setFont(12.0f);
    optionLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(optionLabel);

    // Documentation & Methods
    docTitleLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    docTitleLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(docTitleLabel);

    descLabel.setFont(11.0f);
    descLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    descLabel.setMinimumHorizontalScale(0.9f);
    addAndMakeVisible(descLabel);

    inletOutletLabel.setFont(11.0f);
    inletOutletLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::cyberCyan);
    addAndMakeVisible(inletOutletLabel);

    setSelectedNode(nullptr);
}

void NodeInspectorComponent::setSelectedNode(std::shared_ptr<RelativisticNode> node)
{
    selectedNode = node;
    updateUIForSelectedNode();
}

void NodeInspectorComponent::updateUIForSelectedNode()
{
    posXSlider.onValueChange = nullptr;
    posYSlider.onValueChange = nullptr;
    widthSlider.onValueChange = nullptr;
    heightSlider.onValueChange = nullptr;
    scopeVisibleToggle.onClick = nullptr;
    scopeTypeCombo.onChange = nullptr;
    scopeEngineCombo.onChange = nullptr;

    volSlider.onValueChange = nullptr;
    paramSlider1.onValueChange = nullptr;
    paramSlider2.onValueChange = nullptr;
    optionSelector.onChange = nullptr;

    methodButtons.clear();

    if (!selectedNode)
    {
        nodeTypeLabel.setText("Select a node to inspect properties", juce::dontSendNotification);
        posXLabel.setVisible(false); posXSlider.setVisible(false);
        posYLabel.setVisible(false); posYSlider.setVisible(false);
        widthLabel.setVisible(false); widthSlider.setVisible(false);
        heightLabel.setVisible(false); heightSlider.setVisible(false);
        scopeVisibleToggle.setVisible(false);
        scopeTypeLabel.setVisible(false); scopeTypeCombo.setVisible(false);
        scopeEngineLabel.setVisible(false); scopeEngineCombo.setVisible(false);
        volLabel.setVisible(false); volSlider.setVisible(false);
        paramSlider1.setVisible(false); paramLabel1.setVisible(false);
        paramSlider2.setVisible(false); paramLabel2.setVisible(false);
        optionSelector.setVisible(false); optionLabel.setVisible(false);
        docTitleLabel.setVisible(false); descLabel.setVisible(false);
        inletOutletLabel.setVisible(false);
        return;
    }

    // Bind Geometry with Flexible Ranges
    posXLabel.setVisible(true); posXSlider.setVisible(true);
    posXSlider.setRange(-100000.0, 100000.0, 1.0);
    posXSlider.setValue(selectedNode->xPos, juce::dontSendNotification);
    posXSlider.onValueChange = [this]() {
        if (selectedNode) { selectedNode->xPos = static_cast<float>(posXSlider.getValue()); if (getParentComponent()) getParentComponent()->repaint(); }
    };

    posYLabel.setVisible(true); posYSlider.setVisible(true);
    posYSlider.setRange(-100000.0, 100000.0, 1.0);
    posYSlider.setValue(selectedNode->yPos, juce::dontSendNotification);
    posYSlider.onValueChange = [this]() {
        if (selectedNode) { selectedNode->yPos = static_cast<float>(posYSlider.getValue()); if (getParentComponent()) getParentComponent()->repaint(); }
    };

    widthLabel.setVisible(true); widthSlider.setVisible(true);
    widthSlider.setRange(10.0, 100000.0, 1.0);
    widthSlider.setValue(selectedNode->width, juce::dontSendNotification);
    widthSlider.onValueChange = [this]() {
        if (selectedNode) { selectedNode->width = static_cast<float>(widthSlider.getValue()); if (getParentComponent()) getParentComponent()->repaint(); }
    };

    heightLabel.setVisible(true); heightSlider.setVisible(true);
    heightSlider.setRange(10.0, 100000.0, 1.0);
    heightSlider.setValue(selectedNode->height, juce::dontSendNotification);
    heightSlider.onValueChange = [this]() {
        if (selectedNode) { selectedNode->height = static_cast<float>(heightSlider.getValue()); if (getParentComponent()) getParentComponent()->repaint(); }
    };

    // Bind Scope Controls
    scopeVisibleToggle.setVisible(true);
    scopeVisibleToggle.setToggleState(selectedNode->showRealtimeDisplay, juce::dontSendNotification);
    scopeVisibleToggle.onClick = [this]() {
        if (selectedNode) { selectedNode->showRealtimeDisplay = scopeVisibleToggle.getToggleState(); if (getParentComponent()) getParentComponent()->repaint(); }
    };

    scopeTypeLabel.setVisible(true); scopeTypeCombo.setVisible(true);
    int typeIdx = 1;
    if (selectedNode->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform) typeIdx = 1;
    else if (selectedNode->timeVarMode == RelativisticNode::TimeScopeVariable::SpeedGamma) typeIdx = 2;
    else if (selectedNode->timeVarMode == RelativisticNode::TimeScopeVariable::OffsetTau) typeIdx = 3;
    else if (selectedNode->timeVarMode == RelativisticNode::TimeScopeVariable::CouplingC) typeIdx = 4;
    else if (selectedNode->timeVarMode == RelativisticNode::TimeScopeVariable::MultiTime) typeIdx = 5;
    scopeTypeCombo.setSelectedId(typeIdx, juce::dontSendNotification);
    scopeTypeCombo.onChange = [this]() {
        if (!selectedNode) return;
        int id = scopeTypeCombo.getSelectedId();
        if (id == 1) { selectedNode->displayType = RelativisticNode::ScopeDisplayType::AudioWaveform; }
        else {
            selectedNode->displayType = RelativisticNode::ScopeDisplayType::TimeFrame;
            if (id == 2) selectedNode->timeVarMode = RelativisticNode::TimeScopeVariable::SpeedGamma;
            else if (id == 3) selectedNode->timeVarMode = RelativisticNode::TimeScopeVariable::OffsetTau;
            else if (id == 4) selectedNode->timeVarMode = RelativisticNode::TimeScopeVariable::CouplingC;
            else if (id == 5) selectedNode->timeVarMode = RelativisticNode::TimeScopeVariable::MultiTime;
        }
        if (getParentComponent()) getParentComponent()->repaint();
    };

    scopeEngineLabel.setVisible(true); scopeEngineCombo.setVisible(true);
    int engIdx = 1;
    if (selectedNode->scopeMode == RelativisticNode::ScopeRenderMode::Waveform2D) engIdx = 1;
    else if (selectedNode->scopeMode == RelativisticNode::ScopeRenderMode::ScopeXY) engIdx = 2;
    else if (selectedNode->scopeMode == RelativisticNode::ScopeRenderMode::Scope3D) engIdx = 3;
    scopeEngineCombo.setSelectedId(engIdx, juce::dontSendNotification);
    scopeEngineCombo.onChange = [this]() {
        if (!selectedNode) return;
        int id = scopeEngineCombo.getSelectedId();
        if (id == 1) selectedNode->scopeMode = RelativisticNode::ScopeRenderMode::Waveform2D;
        else if (id == 2) selectedNode->scopeMode = RelativisticNode::ScopeRenderMode::ScopeXY;
        else if (id == 3) selectedNode->scopeMode = RelativisticNode::ScopeRenderMode::Scope3D;
        if (getParentComponent()) getParentComponent()->repaint();
    };

    volLabel.setText("Output Volume (dBFS)", juce::dontSendNotification);
    volLabel.setVisible(true);
    volSlider.setVisible(true);
    volSlider.setRange(-100.0, 6.0, 0.1);
    volSlider.setValue(selectedNode->getVolumeDb(), juce::dontSendNotification);
    volSlider.setTextValueSuffix(" dB");
    volSlider.onValueChange = [this]() {
        if (selectedNode) selectedNode->setVolumeDb(static_cast<float>(volSlider.getValue()));
    };

    docTitleLabel.setVisible(true);
    descLabel.setVisible(true);
    inletOutletLabel.setVisible(true);

    std::string sym = selectedNode->getSymbol();
    juce::String titleText = "[" + sym + "] \u2014 " + selectedNode->getLabel();
    if (selectedNode->getRmsLevel() > 0.0001f || selectedNode->getPeakLevel() > 0.0001f)
    {
        titleText += "  [RMS: " + juce::String(selectedNode->getRmsLevel(), 3) + " Peak: " + juce::String(selectedNode->getPeakLevel(), 3) + "]";
    }
    nodeTypeLabel.setText(titleText, juce::dontSendNotification);

    std::vector<std::string> templateMsgs;

    if (sym == "ladder~")
    {
        paramLabel1.setText("Cutoff Freq (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-1000000.0, 1000000.0, 1.0);
        paramSlider1.setValue(1000.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("cutoff " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Resonance Q", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(-1000.0, 1000.0, 0.01);
        paramSlider2.setValue(0.5, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("res " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("4-Pole Moog VA Ladder Filter. Modulates spectrum via time dilation & cutoff control.", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg | In 1: Time | In 2: Audio~ | Out 0: Msg | Out 1: Time | Out 2: Audio~", juce::dontSendNotification);

        templateMsgs = { "cutoff 1200", "cutoff 3500", "res 0.75", "res 0.2" };
    }
    else if (sym == "time.lfo~" || sym == "time.lfo")
    {
        paramLabel1.setText("LFO Rate (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-100000.0, 100000.0, 0.01);
        paramSlider1.setValue(0.5, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("rate " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("LFO Depth (Gamma Mod)", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(-100000.0, 100000.0, 0.01);
        paramSlider2.setValue(0.85, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("depth " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Relativistic Low Frequency Oscillator modulating local proper time velocity gamma(t).", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Time | In 1: Rate (Msg) | Out 0: Time", juce::dontSendNotification);

        templateMsgs = { "rate 0.5", "rate -0.5", "depth 0.85", "depth -0.5" };
    }
    else if (sym == "osc~")
    {
        paramLabel1.setText("Frequency (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-1000000.0, 1000000.0, 1.0);
        paramSlider1.setValue(440.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("freq " + std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);

        optionLabel.setText("Waveform Shape", juce::dontSendNotification);
        optionLabel.setVisible(true);
        optionSelector.clear();
        optionSelector.addItem("Sine (sin)", 1);
        optionSelector.addItem("Sawtooth (saw)", 2);
        optionSelector.addItem("Square (square)", 3);
        optionSelector.addItem("Triangle (tri)", 4);
        optionSelector.setSelectedId(1, juce::dontSendNotification);
        optionSelector.setVisible(true);
        optionSelector.onChange = [this]() {
            int id = optionSelector.getSelectedId();
            std::string w = (id == 2) ? "saw" : ((id == 3) ? "square" : ((id == 4) ? "tri" : "sin"));
            if (selectedNode) selectedNode->receiveMessage("wave " + w);
        };

        descLabel.setText("Anti-aliased PolyBLEP Oscillator. Doppler phase accumulation modulated by local proper time gamma.", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg/Freq | In 1: Time | Out 0: Audio~", juce::dontSendNotification);

        templateMsgs = { "freq 440", "freq -440", "wave saw", "wave tri", "wave square", "phase 0.0" };
    }
    else if (sym == "drive~" || sym == "saturate~")
    {
        paramLabel1.setText("Saturation Drive", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-1000.0, 1000.0, 0.1);
        paramSlider1.setValue(2.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("drive " + std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Hyperbolic WaveShaper distortion node adding tube harmonics to signal.", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg/Audio | Out 0: Audio~", juce::dontSendNotification);

        templateMsgs = { "drive 2.5", "drive -2.5", "drive 1.0" };
    }
    else if (sym == "time.warp")
    {
        paramLabel1.setText("Time Warp Factor (\u03b3)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-100000.0, 100000.0, 0.05);
        paramSlider1.setValue(1.5, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("factor " + std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Relativistic Time Warp node modulating local proper time flow speed gamma.", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg/Warp | Out 0: Time Frame", juce::dontSendNotification);

        templateMsgs = { "warp 1.5", "warp -1.0", "warp 0.5", "warp 2.0" };
    }
    else if (sym == "seq")
    {
        paramLabel1.setText("Sequencer BPM", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-100000.0, 100000.0, 1.0);
        paramSlider1.setValue(120.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("bpm " + std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Relativistic Step Sequencer. Advances notes according to local proper time gamma.", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg/Time | Out 0: Note | Out 1: Gate", juce::dontSendNotification);

        templateMsgs = { "notes 60 62 64 65 67 69 71 72", "notes 60 63 67 70 72 75 74 70", "notes 36 38 40 43 45", "bpm 140", "bpm 90" };
    }
    else if (sym == "pluck~")
    {
        paramLabel1.setText("Pitch Frequency (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-1000000.0, 1000000.0, 1.0);
        paramSlider1.setValue(220.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("set " + std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Karplus-Strong Physical String Model with lossy feedback delay loop.", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg/Pitch | Out 0: Audio~", juce::dontSendNotification);

        templateMsgs = { "play", "set 220", "set -220", "set 110" };
    }
    else if (sym == "out~")
    {
        paramLabel1.setText("Master Volume (dBFS)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-100.0, 6.0, 0.1);
        paramSlider1.setValue(selectedNode->getVolumeDb(), juce::dontSendNotification);
        paramSlider1.setTextValueSuffix(" dB");
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->setVolumeDb(static_cast<float>(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Master Stereo Output & Monitoring Node with log-scale (-\u221e to +6 dBFS) volume control.", juce::dontSendNotification);
        templateMsgs = { "vol 0", "vol -6", "vol -12", "vol -24", "vol -inf", "play", "stop" };
    }
    else
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Pure Data-style relational node object.", juce::dontSendNotification);
        templateMsgs = { "play", "stop", "reset" };
    }

    // Dynamic Inlet & Outlet Description Format for ALL Node Types
    std::ostringstream ioSs;
    ioSs << "INLETS (" << selectedNode->getInlets().size() << "):\n";
    for (size_t i = 0; i < selectedNode->getInlets().size(); ++i)
    {
        const auto& in = selectedNode->getInlets()[i];
        std::string typeStr = (in.dataType == PortDataType::Audio) ? "Audio~ (Cyan)" :
                              ((in.dataType == PortDataType::Time) ? "Time (Royal Violet)" : "Message (Gold)");
        ioSs << "  In " << i << ": " << in.name << " \u2014 " << typeStr << "\n";
    }
    ioSs << "\nOUTLETS (" << selectedNode->getOutlets().size() << "):\n";
    for (size_t o = 0; o < selectedNode->getOutlets().size(); ++o)
    {
        const auto& out = selectedNode->getOutlets()[o];
        std::string typeStr = (out.dataType == PortDataType::Audio) ? "Audio~ (Cyan)" :
                              ((out.dataType == PortDataType::Time) ? "Time (Royal Violet)" : "Message (Gold)");
        ioSs << "  Out " << o << ": " << out.name << " \u2014 " << typeStr << "\n";
    }

    inletOutletLabel.setText(ioSs.str(), juce::dontSendNotification);

    // Generate One-Click Method Buttons
    for (const auto& msgStr : templateMsgs)
    {
        auto btn = std::make_unique<juce::TextButton>("+ msg " + msgStr);
        btn->onClick = [this, msgStr]() {
            if (selectedNode && onSpawnMessageBox)
            {
                onSpawnMessageBox(selectedNode->getId(), msgStr);
            }
        };
        addAndMakeVisible(*btn);
        methodButtons.push_back(std::move(btn));
    }

    resized();
}

void NodeInspectorComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll(CarbonGoldLookAndFeel::slatePanel);

    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.3f));
    g.drawHorizontalLine(32, 0.0f, b.getWidth());
}

void NodeInspectorComponent::resized()
{
    titleLabel.setBounds(10, 5, getWidth() - 20, 22);
    nodeTypeLabel.setBounds(10, 32, getWidth() - 20, 20);

    int y = 58;
    int w = getWidth() - 20;
    int halfW = (w - 10) / 2;

    if (posXLabel.isVisible())
    {
        posXLabel.setBounds(10, y, halfW, 16);
        posYLabel.setBounds(15 + halfW, y, halfW, 16);
        y += 18;

        posXSlider.setBounds(10, y, halfW, 22);
        posYSlider.setBounds(15 + halfW, y, halfW, 22);
        y += 26;

        widthLabel.setBounds(10, y, halfW, 16);
        heightLabel.setBounds(15 + halfW, y, halfW, 16);
        y += 18;

        widthSlider.setBounds(10, y, halfW, 22);
        heightSlider.setBounds(15 + halfW, y, halfW, 22);
        y += 28;
    }

    if (scopeVisibleToggle.isVisible())
    {
        scopeVisibleToggle.setBounds(10, y, w, 22); y += 24;

        scopeTypeLabel.setBounds(10, y, w, 16); y += 18;
        scopeTypeCombo.setBounds(10, y, w, 24); y += 28;

        scopeEngineLabel.setBounds(10, y, w, 16); y += 18;
        scopeEngineCombo.setBounds(10, y, w, 24); y += 28;
    }

    if (volLabel.isVisible())
    {
        volLabel.setBounds(10, y, w, 18); y += 20;
        volSlider.setBounds(10, y, w, 24); y += 30;
    }

    if (paramLabel1.isVisible())
    {
        paramLabel1.setBounds(10, y, w, 18); y += 20;
        paramSlider1.setBounds(10, y, w, 24); y += 30;
    }

    if (paramLabel2.isVisible())
    {
        paramLabel2.setBounds(10, y, w, 18); y += 20;
        paramSlider2.setBounds(10, y, w, 24); y += 30;
    }

    if (optionLabel.isVisible())
    {
        optionLabel.setBounds(10, y, w, 18); y += 20;
        optionSelector.setBounds(10, y, w, 26); y += 32;
    }

    y += 10;
    if (docTitleLabel.isVisible())
    {
        docTitleLabel.setBounds(10, y, w, 20); y += 22;
        descLabel.setBounds(10, y, w, 36); y += 38;

        int ioLines = selectedNode ? static_cast<int>(selectedNode->getInlets().size() + selectedNode->getOutlets().size() + 3) : 4;
        int ioHeight = ioLines * 15;
        inletOutletLabel.setBounds(10, y, w, ioHeight); y += ioHeight + 8;

        for (auto& btn : methodButtons)
        {
            btn->setBounds(10, y, w, 24);
            y += 28;
        }
    }
}

} // namespace TimeDilationDAW
