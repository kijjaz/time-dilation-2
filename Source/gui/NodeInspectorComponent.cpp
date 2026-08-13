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
    volSlider.onValueChange = nullptr;
    paramSlider1.onValueChange = nullptr;
    paramSlider2.onValueChange = nullptr;
    optionSelector.onChange = nullptr;

    methodButtons.clear();

    if (!selectedNode)
    {
        nodeTypeLabel.setText("Select a node to inspect properties", juce::dontSendNotification);
        volLabel.setVisible(false);
        volSlider.setVisible(false);
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);
        docTitleLabel.setVisible(false);
        descLabel.setVisible(false);
        inletOutletLabel.setVisible(false);
        return;
    }

    volLabel.setVisible(true);
    volSlider.setVisible(true);
    volSlider.setRange(-4.0, 4.0, 0.01);
    volSlider.setValue(selectedNode->getOutputVolume(), juce::dontSendNotification);
    volSlider.onValueChange = [this]() {
        if (selectedNode) selectedNode->setOutputVolume(static_cast<float>(volSlider.getValue()));
    };

    docTitleLabel.setVisible(true);
    descLabel.setVisible(true);
    inletOutletLabel.setVisible(true);

    std::string sym = selectedNode->getSymbol();
    nodeTypeLabel.setText("[" + sym + "] — " + selectedNode->getLabel(), juce::dontSendNotification);

    std::vector<std::string> templateMsgs;

    if (sym == "ladder~")
    {
        paramLabel1.setText("Cutoff Freq (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-20000.0, 20000.0, 1.0);
        paramSlider1.setValue(1000.0, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("cutoff " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("Resonance Q", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(-5.0, 5.0, 0.01);
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
        paramSlider1.setRange(-50.0, 50.0, 0.01);
        paramSlider1.setValue(0.5, juce::dontSendNotification);
        paramSlider1.setVisible(true);
        paramSlider1.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("rate " + std::to_string(paramSlider1.getValue()));
        };

        paramLabel2.setText("LFO Depth (Gamma Mod)", juce::dontSendNotification);
        paramLabel2.setVisible(true);
        paramSlider2.setRange(-10.0, 10.0, 0.01);
        paramSlider2.setValue(0.85, juce::dontSendNotification);
        paramSlider2.setVisible(true);
        paramSlider2.onValueChange = [this]() {
            if (selectedNode) selectedNode->receiveMessage("depth " + std::to_string(paramSlider2.getValue()));
        };

        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Relativistic Low Frequency Oscillator modulating local proper time velocity gamma(t).", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg | In 1: Time | In 2: Rate~ | Out 0: Msg | Out 1: Time | Out 2: Audio~", juce::dontSendNotification);

        templateMsgs = { "rate 0.5", "rate -0.5", "depth 0.85", "depth -0.5" };
    }
    else if (sym == "osc~")
    {
        paramLabel1.setText("Frequency (Hz)", juce::dontSendNotification);
        paramLabel1.setVisible(true);
        paramSlider1.setRange(-20000.0, 20000.0, 1.0);
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
        paramSlider1.setRange(-20.0, 20.0, 0.1);
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
        paramSlider1.setRange(-10.0, 10.0, 0.05);
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
        paramSlider1.setRange(-400.0, 400.0, 1.0);
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
        paramSlider1.setRange(-2000.0, 2000.0, 1.0);
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
    else
    {
        paramSlider1.setVisible(false);
        paramLabel1.setVisible(false);
        paramSlider2.setVisible(false);
        paramLabel2.setVisible(false);
        optionSelector.setVisible(false);
        optionLabel.setVisible(false);

        descLabel.setText("Pure Data-style relational node object.", juce::dontSendNotification);
        inletOutletLabel.setText("In 0: Msg | Out 0: Msg", juce::dontSendNotification);

        templateMsgs = { "play", "stop", "reset" };
    }

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
    nodeTypeLabel.setBounds(10, 36, getWidth() - 20, 20);

    int y = 65;
    int w = getWidth() - 20;

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
        inletOutletLabel.setBounds(10, y, w, 20); y += 26;

        for (auto& btn : methodButtons)
        {
            btn->setBounds(10, y, w, 24);
            y += 28;
        }
    }
}

} // namespace TimeDilationDAW
