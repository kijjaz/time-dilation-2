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
    descLabel.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(descLabel);

    inletOutletLabel.setFont(11.0f);
    inletOutletLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::cyberCyan);
    inletOutletLabel.setJustificationType(juce::Justification::topLeft);
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

    std::string sym = selectedNode->getSymbol();
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);
    bool isControlNode = (sym == "msg" || sym == "message" || sym == "bang" || sym == "bng" ||
                          sym == "toggle" || sym == "tgl" || sym == "number" || sym == "num" ||
                          sym == "symbol" || sym == "sym" || sym == "radio" || sym == "hradio" ||
                          sym == "vradio" || sym == "display" || sym == "disp" || sym == "print");

    if (isControlNode)
    {
        // Control & message objects do not process audio signals: hide scope & volume controls
        scopeVisibleToggle.setVisible(false);
        scopeTypeLabel.setVisible(false); scopeTypeCombo.setVisible(false);
        scopeEngineLabel.setVisible(false); scopeEngineCombo.setVisible(false);
        volLabel.setVisible(false);
        volSlider.setVisible(false);
    }
    else
    {
        // Bind Scope Controls for Audio / Time DSP Nodes
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

        if (sym == "out~")
        {
            // Master Output Node uses Volume in dB (e.g. -6.0 dB)
            volLabel.setText("Master Output Volume (dB)", juce::dontSendNotification);
            volLabel.setVisible(true);
            volSlider.setVisible(true);
            volSlider.setRange(-100.0, 6.0, 0.1);
            volSlider.setValue(selectedNode->getVolumeDb(), juce::dontSendNotification);
            volSlider.setTextValueSuffix(" dB");
            volSlider.onValueChange = [this]() {
                if (selectedNode) selectedNode->setVolumeDb(static_cast<float>(volSlider.getValue()));
            };
        }
        else
        {
            // General Audio / DSP Nodes use Linear Gain Multiplier (e.g. 1.0x)
            volLabel.setText("Output Level (Gain Multiplier)", juce::dontSendNotification);
            volLabel.setVisible(true);
            volSlider.setVisible(true);
            volSlider.setRange(0.0, 4.0, 0.01);
            volSlider.setValue(selectedNode->getOutputVolume(), juce::dontSendNotification);
            volSlider.setTextValueSuffix("x");
            volSlider.onValueChange = [this]() {
                if (selectedNode) selectedNode->setOutputVolume(static_cast<float>(volSlider.getValue()));
            };
        }
    }

    docTitleLabel.setVisible(true);
    descLabel.setVisible(true);
    inletOutletLabel.setVisible(true);

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
    else if (sym == "delay~")
    {
        paramLabel1.setText("Delay Time (sec)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(0.001, 5.0, 0.001);
        paramSlider1.setValue(0.35, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("time " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Feedback Gain", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(0.0, 0.99, 0.01);
        paramSlider2.setValue(0.5, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("feedback " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Feedback Delay Line with continuous cubic Hermite interpolation and Doppler time modulation.", juce::dontSendNotification);
        templateMsgs = { "time 0.25", "time 0.5", "feedback 0.6", "feedback 0.2" };
    }
    else if (sym == "svf~")
    {
        paramLabel1.setText("Cutoff Freq (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(20.0, 20000.0, 1.0);
        paramSlider1.setValue(1000.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("cutoff " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Resonance (Q)", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(0.1, 20.0, 0.01);
        paramSlider2.setValue(0.707, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("q " + std::to_string(paramSlider2.getValue()));
        };

        optionLabel.setText("Filter Type", juce::dontSendNotification);
        optionLabel.setVisible(true);
        optionSelector.clear();
        optionSelector.addItem("Lowpass (LP)", 1);
        optionSelector.addItem("Highpass (HP)", 2);
        optionSelector.addItem("Bandpass (BP)", 3);
        optionSelector.addItem("Notch (Notch)", 4);
        optionSelector.setSelectedId(1, juce::dontSendNotification);
        optionSelector.setVisible(true);
        optionSelector.onChange = [this]() {
            int id = optionSelector.getSelectedId();
            std::string t = (id == 2) ? "hp" : ((id == 3) ? "bp" : ((id == 4) ? "notch" : "lp"));
            if (selectedNode) selectedNode->receiveMessage("type " + t);
        };

        descLabel.setText("State Variable Filter offering simultaneous Lowpass, Highpass, Bandpass, and Notch responses.", juce::dontSendNotification);
        templateMsgs = { "cutoff 800", "cutoff 2500", "q 2.0", "q 0.707", "type lp", "type hp", "type bp", "type notch" };
    }
    else if (sym == "time.lorentz~" || sym == "time.lorentz")
    {
        paramLabel1.setText("Relativistic Velocity (v/c)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-0.99, 0.99, 0.01);
        paramSlider1.setValue(0.5, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("vel " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Center Freq (Hz)", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(20.0, 20000.0, 1.0);
        paramSlider2.setValue(1200.0, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("freq " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Lorentz Velocity Filter. Warps formant bandwidth and relativistic boost according to velocity v.", juce::dontSendNotification);
        templateMsgs = { "vel 0.5", "vel 0.85", "vel -0.6", "freq 1500" };
    }
    else if (sym == "time.grav.osc~" || sym == "time.grav~" || sym == "grav.osc~" || sym == "grav.osc")
    {
        paramLabel1.setText("Gravitational Mass (M)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(0.0, 10.0, 0.1);
        paramSlider1.setValue(1.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("mass " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Orbital Radius (R)", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(0.1, 20.0, 0.1);
        paramSlider2.setValue(2.0, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("radius " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Gravitational Redshift Oscillator. Frequency is redshifted according to Einstein's General Relativity.", juce::dontSendNotification);
        templateMsgs = { "mass 1.5", "radius 3.0", "freq 440", "freq 220" };
    }
    else if (sym == "mtof" || sym == "mtof~")
    {
        paramLabel1.setText("MIDI Note Number", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(0.0, 127.0, 1.0);
        paramSlider1.setValue(60.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage(std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Converts MIDI Note Number (m) to Frequency in Hz using: f = 440 * 2^((m - 69)/12).", juce::dontSendNotification);
        templateMsgs = { "60", "62", "64", "65", "67", "69", "71", "72" };
    }
    else if (sym == "ftom" || sym == "ftom~")
    {
        paramLabel1.setText("Frequency in Hz", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(20.0, 20000.0, 1.0);
        paramSlider1.setValue(440.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage(std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Converts Frequency in Hz (f) to MIDI Note Number using: m = 69 + 12 * log2(f / 440).", juce::dontSendNotification);
        templateMsgs = { "440.0", "261.63", "220.0", "880.0" };
    }
    else if (sym == "number" || sym == "num")
    {
        paramLabel1.setText("Number Value", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-100000.0, 100000.0, 0.1);
        auto numNode = std::dynamic_pointer_cast<NumberNode>(selectedNode);
        paramSlider1.setValue(numNode ? numNode->getValue() : 0.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage(std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Numeric parameter box for entering, editing, or displaying integer and float numbers.", juce::dontSendNotification);
        templateMsgs = { "440", "120", "0.5", "-6.0" };
    }
    else if (sym == "toggle" || sym == "tgl")
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);

        optionLabel.setText("Toggle State", juce::dontSendNotification);
        optionLabel.setVisible(true);
        optionSelector.clear();
        optionSelector.addItem("OFF [  ] (0)", 1);
        optionSelector.addItem("ON [X] (1)", 2);
        auto tNode = std::dynamic_pointer_cast<ToggleNode>(selectedNode);
        optionSelector.setSelectedId(tNode && tNode->getState() ? 2 : 1, juce::dontSendNotification);
        optionSelector.setVisible(true);
        optionSelector.onChange = [this, tNode]() {
            if (tNode) tNode->setState(optionSelector.getSelectedId() == 2);
            if (getParentComponent()) getParentComponent()->repaint();
        };

        descLabel.setText("Boolean Toggle Box. Outputs 1 (ON) or 0 (OFF). Toggles state when clicked or banged.", juce::dontSendNotification);
        templateMsgs = { "1", "0", "bang", "toggle" };
    }
    else if (sym == "radio" || sym == "hradio" || sym == "vradio")
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);

        auto rNode = std::dynamic_pointer_cast<RadioNode>(selectedNode);
        int opts = rNode ? rNode->getNumOptions() : 4;
        optionLabel.setText("Selected Radio Option", juce::dontSendNotification);
        optionLabel.setVisible(true);
        optionSelector.clear();
        for (int i = 0; i < opts; ++i)
        {
            optionSelector.addItem("Option " + std::to_string(i), i + 1);
        }
        optionSelector.setSelectedId((rNode ? rNode->getSelectedIndex() : 0) + 1, juce::dontSendNotification);
        optionSelector.setVisible(true);
        optionSelector.onChange = [this, rNode]() {
            if (rNode) rNode->selectOption(optionSelector.getSelectedId() - 1);
            if (getParentComponent()) getParentComponent()->repaint();
        };

        descLabel.setText("Radio Button Strip. Emits the selected index (0, 1, 2, ...) downstream when clicked.", juce::dontSendNotification);
        templateMsgs = { "0", "1", "2", "3" };
    }
    else if (sym == "pack~" || sym == "bundle~" || sym == "join~")
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        auto pNode = std::dynamic_pointer_cast<PackNode>(selectedNode);
        int chs = pNode ? pNode->getNumChannels() : 2;
        descLabel.setText("Multi-Channel Audio Bundler. Bundles " + std::to_string(chs) + " discrete mono audio inputs into 1 multichannel audio cable.", juce::dontSendNotification);
        templateMsgs = { "channels " + std::to_string(chs) };
    }
    else if (sym == "unpack~" || sym == "unbundle~" || sym == "split~")
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        auto uNode = std::dynamic_pointer_cast<UnpackNode>(selectedNode);
        int chs = uNode ? uNode->getNumChannels() : 2;
        descLabel.setText("Multi-Channel Audio Splitter. Splits 1 multichannel audio cable into " + std::to_string(chs) + " discrete mono audio outlets.", juce::dontSendNotification);
        templateMsgs = { "channels " + std::to_string(chs) };
    }
    else if (sym == "reverb~" || sym == "freeverb~" || sym == "rev1~")
    {
        paramLabel1.setText("Room Size (0.0 - 1.0)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(0.0, 1.0, 0.01);
        paramSlider1.setValue(0.7, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("room " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Wet Mix (0.0 - 1.0)", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(0.0, 1.0, 0.01);
        paramSlider2.setValue(0.35, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("wet " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Stereo Algorithmic Reverb with 8-comb / 4-allpass Freeverb topology and HF damping.", juce::dontSendNotification);
        templateMsgs = { "room 0.8", "damp 0.5", "wet 0.4", "wet 0.1", "dry 0.9" };
    }
    else if (sym == "kick~" || sym == "drum.kick~")
    {
        paramLabel1.setText("Base Pitch (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(20.0, 120.0, 0.5);
        paramSlider1.setValue(50.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("pitch " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Decay Time (sec)", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(0.05, 1.5, 0.01);
        paramSlider2.setValue(0.35, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("decay " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Analog Sub-Bass Kick Drum with pitch envelope sweep and punchy transient click.", juce::dontSendNotification);
        templateMsgs = { "play", "pitch 45", "pitch 60", "decay 0.25", "decay 0.6" };
    }
    else if (sym == "snare~" || sym == "drum.snare~")
    {
        paramLabel1.setText("Tone Frequency (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(100.0, 350.0, 1.0);
        paramSlider1.setValue(185.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("tone " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Snappy Noise Amount", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(0.0, 1.0, 0.01);
        paramSlider2.setValue(0.65, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("snappy " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Analog Snare Drum with dual harmonic body resonators and filtered noise burst.", juce::dontSendNotification);
        templateMsgs = { "play", "tone 200", "snappy 0.8", "decay 0.2", "decay 0.4" };
    }
    else if (sym == "hihat~" || sym == "drum.hat~")
    {
        paramLabel1.setText("Decay Time (sec)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(0.02, 0.8, 0.01);
        paramSlider1.setValue(0.08, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("decay " + std::to_string(paramSlider1.getValue()));
        };

        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Metallic Multi-Pulse Closed/Open Hi-Hat synthesizer.", juce::dontSendNotification);
        templateMsgs = { "play", "close", "open", "decay 0.05", "decay 0.35" };
    }
    else if (sym == "noise~")
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);

        optionLabel.setText("Noise Mode", juce::dontSendNotification);
        optionLabel.setVisible(true);
        optionSelector.clear();
        optionSelector.addItem("White Noise", 1);
        optionSelector.addItem("Pink Noise (1/f)", 2);
        optionSelector.setSelectedId(1, juce::dontSendNotification);
        optionSelector.setVisible(true);
        optionSelector.onChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage(optionSelector.getSelectedId() == 2 ? "pink" : "white");
        };

        descLabel.setText("White & 1/f Pink Noise generator for synth percussion and atmospheric sound design.", juce::dontSendNotification);
        templateMsgs = { "white", "pink" };
    }
    else if (sym == "table" || sym == "tabread~")
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Shared Audio Sample Buffer Array & Reader for custom waveform playback and sequencing.", juce::dontSendNotification);
        templateMsgs = { "size 44100", "size 88200", "clear" };
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
        templateMsgs = { "play", "stop", "reset", "bang" };
    }

    auto getPortFunctionDesc = [](const std::string& symbol, bool isOutlet, int index, const std::string& portName) -> std::string {
        std::string sym = symbol;
        std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

        if (sym == "osc~") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (freq <hz>, wave <shape>, vol <db>)";
                if (index == 1) return "Relativistic Time Input (modulates pitch via Doppler \u03b3)";
                if (index == 2) return "Frequency Modulation (FM) Audio Input (~)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Time Frame Output (\u03b3)";
                if (index == 2) return "Anti-aliased Audio Output (~)";
            }
        }
        else if (sym == "ladder~") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (cutoff <hz>, res <q>, vol <db>)";
                if (index == 1) return "Relativistic Time Input (modulates filter cutoff)";
                if (index == 2) return "Audio Signal Input (~)";
                if (index == 3) return "Cutoff Audio Modulation Input (~)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Time Frame Output (\u03b3)";
                if (index == 2) return "Filtered Moog 4-Pole Audio Output (~)";
            }
        }
        else if (sym == "drive~" || sym == "saturate~") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (drive <gain>, vol <db>)";
                if (index == 1) return "Relativistic Time Input";
                if (index == 2) return "Audio Signal Input (~)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Time Frame Output (\u03b3)";
                if (index == 2) return "Tube Saturated Audio Output (~)";
            }
        }
        else if (sym == "pluck~") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (set <freq>, play, vol <db>)";
                if (index == 1) return "Relativistic Time Input";
                if (index == 2) return "Exciter Trigger Audio Input (~)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Time Frame Output (\u03b3)";
                if (index == 2) return "Karplus-Strong String Audio Output (~)";
            }
        }
        else if (sym == "out~") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (vol <dbFS>, play, stop)";
                if (index == 1) return "Left Channel Audio Input (Strict Channel 0)";
                if (index == 2) return "Right Channel Audio Input (Strict Channel 1)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Stereo Audio Pass-Through Output (~)";
            }
        }
        else if (sym == "meter~" || sym == "vu~") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (peak, rms, lufs)";
                if (index == 1) return "Audio Input to Monitor (~)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Pass-Through Audio Output (~)";
            }
        }
        else if (sym == "spectrogram~" || sym == "spec~") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (freeze, resume, clear)";
                if (index == 1) return "Audio Input for FFT Analysis (~)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Pass-Through Audio Output (~)";
            }
        }
        else if (sym == "time.lfo~" || sym == "time.lfo") {
            if (!isOutlet) {
                if (index == 0) return "Relativistic Time Input to Modulate";
                if (index == 1) return "Control Messages (rate <hz>, depth <gamma>)";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Modulated Relativistic Time Output (\u03b3)";
            }
        }
        else if (sym == "time.warp") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (factor <gamma>, warp <gamma>)";
                if (index == 1) return "Relativistic Time Input";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "Time-Warped Relativistic Output (\u03b3)";
            }
        }
        else if (sym == "seq") {
            if (!isOutlet) {
                if (index == 0) return "Control Messages (notes <list>, bpm <val>)";
                if (index == 1) return "Relativistic Time Clock Input";
            } else {
                if (index == 0) return "Message Output";
                if (index == 1) return "MIDI Pitch Audio Outlet (Hz/Note)";
                if (index == 2) return "Gate Trigger Audio Outlet (0 or 1)";
            }
        }
        return isOutlet ? ("Output Port: " + portName) : ("Input Port: " + portName);
    };

    // Dynamic Inlet & Outlet Description Format for ALL Node Types
    std::ostringstream ioSs;
    ioSs << "INLETS (" << selectedNode->getInlets().size() << "):\n";
    for (size_t i = 0; i < selectedNode->getInlets().size(); ++i)
    {
        const auto& in = selectedNode->getInlets()[i];
        std::string typeStr = (in.dataType == PortDataType::Audio) ? "Audio~ (Cyan)" :
                              ((in.dataType == PortDataType::Time) ? "Time (Royal Violet)" : "Message (Gold)");
        std::string fDesc = getPortFunctionDesc(sym, false, static_cast<int>(i), in.name);
        ioSs << "  In " << i << " [" << in.name << "] (" << typeStr << "): " << fDesc << "\n";
    }

    ioSs << "\nOUTLETS (" << selectedNode->getOutlets().size() << "):\n";
    for (size_t o = 0; o < selectedNode->getOutlets().size(); ++o)
    {
        const auto& out = selectedNode->getOutlets()[o];
        std::string typeStr = (out.dataType == PortDataType::Audio) ? "Audio~ (Cyan)" :
                              ((out.dataType == PortDataType::Time) ? "Time (Royal Violet)" : "Message (Gold)");
        std::string fDesc = getPortFunctionDesc(sym, true, static_cast<int>(o), out.name);
        ioSs << "  Out " << o << " [" << out.name << "] (" << typeStr << "): " << fDesc << "\n";
    }

    if (!templateMsgs.empty())
    {
        ioSs << "\nSUPPORTED MESSAGES & COMMANDS:\n";
        for (const auto& msg : templateMsgs)
        {
            ioSs << "  \u2022 " << msg << "\n";
        }
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

        // Accurate multi-line text height calculation
        auto getDynamicTextHeight = [](const juce::String& text, const juce::Font& font, int width) -> int {
            if (text.isEmpty() || width <= 10) return 20;
            juce::AttributedString as(text);
            as.setFont(font);
            as.setWordWrap(juce::AttributedString::WordWrap::byWord);
            juce::TextLayout tl;
            tl.createLayout(as, static_cast<float>(width));
            return static_cast<int>(std::ceil(tl.getHeight())) + 12;
        };

        int descH = getDynamicTextHeight(descLabel.getText(), descLabel.getFont(), w);
        descLabel.setBounds(10, y, w, descH);
        y += descH + 10;

        int ioH = getDynamicTextHeight(inletOutletLabel.getText(), inletOutletLabel.getFont(), w);
        inletOutletLabel.setBounds(10, y, w, ioH);
        y += ioH + 12;

        for (auto& btn : methodButtons)
        {
            btn->setBounds(10, y, w, 26);
            y += 30;
        }
    }

    int totalRequiredH = y + 30;
    if (getHeight() != totalRequiredH)
    {
        setSize(getWidth(), totalRequiredH);
    }
}

} // namespace TimeDilationDAW
