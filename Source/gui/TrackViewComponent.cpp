#include "TrackViewComponent.h"
#include "CarbonGoldLookAndFeel.h"

namespace TimeDilationDAW
{

TrackLaneComponent::TrackLaneComponent(int id, const std::string& name, std::shared_ptr<RelativisticNode> node)
    : trackId(id), trackName(name), sourceNode(node)
{
    nameLabel.setText("Track " + std::to_string(id) + ": " + trackName, juce::dontSendNotification);
    nameLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, CarbonGoldLookAndFeel::goldAccent);
    addAndMakeVisible(nameLabel);

    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8, juce::dontSendNotification);
    addAndMakeVisible(volumeSlider);

    volumeLabel.setText("Vol", juce::dontSendNotification);
    volumeLabel.setFont(11.0f);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(volumeLabel);

    if (sourceNode)
    {
        std::string sym = sourceNode->getSymbol();
        if (sym == "osc~")
        {
            param1Label.setText("Freq (Hz)", juce::dontSendNotification);
            param1Slider.setRange(20.0, 5000.0, 1.0);
            param1Slider.setValue(440.0, juce::dontSendNotification);
            param1Slider.onValueChange = [this]() {
                if (sourceNode) sourceNode->receiveMessage("freq " + std::to_string(param1Slider.getValue()));
            };
        }
        else if (sym == "ladder~")
        {
            param1Label.setText("Cutoff (Hz)", juce::dontSendNotification);
            param1Slider.setRange(20.0, 20000.0, 1.0);
            param1Slider.setValue(1000.0, juce::dontSendNotification);
            param1Slider.onValueChange = [this]() {
                if (sourceNode) sourceNode->receiveMessage("cutoff " + std::to_string(param1Slider.getValue()));
            };
        }
        else if (sym == "drive~")
        {
            param1Label.setText("Drive", juce::dontSendNotification);
            param1Slider.setRange(1.0, 10.0, 0.1);
            param1Slider.setValue(2.0, juce::dontSendNotification);
            param1Slider.onValueChange = [this]() {
                if (sourceNode) sourceNode->receiveMessage("drive " + std::to_string(param1Slider.getValue()));
            };
        }
        else if (sym == "time.warp" || sym == "time.warp~")
        {
            param1Label.setText("Warp \u03b3", juce::dontSendNotification);
            param1Slider.setRange(0.1, 8.0, 0.05);
            param1Slider.setValue(1.5, juce::dontSendNotification);
            param1Slider.onValueChange = [this]() {
                if (sourceNode) sourceNode->receiveMessage("factor " + std::to_string(param1Slider.getValue()));
            };
        }
        else
        {
            param1Label.setText("Param 1", juce::dontSendNotification);
            param1Slider.setRange(0.0, 1.0, 0.01);
        }

        param1Slider.setSliderStyle(juce::Slider::LinearHorizontal);
        param1Slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
        addAndMakeVisible(param1Slider);
        param1Label.setFont(11.0f);
        param1Label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(param1Label);

        // Bi-directional parameter callback listener
        sourceNode->onMessageEmitted = [this](const std::string& msg) {
            juce::String s(msg);
            auto tokens = juce::StringArray::fromTokens(s, " ", "");
            if (tokens.size() >= 2)
            {
                double v = tokens[1].getDoubleValue();
                updateParamSliderFromNode(v);
            }
        };
    }

    addAndMakeVisible(muteButton);
    addAndMakeVisible(soloButton);
    addAndMakeVisible(inspectPatchButton);

    inspectPatchButton.onClick = [this]() {
        if (sourceNode && onOpenPatchNode)
        {
            onOpenPatchNode(sourceNode->getId());
        }
    };
}

void TrackLaneComponent::updateParamSliderFromNode(double val)
{
    juce::MessageManager::callAsync([this, val]() {
        param1Slider.setValue(val, juce::dontSendNotification);
    });
}

void TrackLaneComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll(CarbonGoldLookAndFeel::slatePanel);

    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.2f));
    g.drawRoundedRectangle(b.reduced(1.0f), 4.0f, 1.0f);
}

void TrackLaneComponent::resized()
{
    int x = 10;
    nameLabel.setBounds(x, 8, 160, 22); x += 170;

    volumeLabel.setBounds(x, 10, 30, 18); x += 32;
    volumeSlider.setBounds(x, 8, 140, 22); x += 150;

    if (param1Slider.isVisible())
    {
        param1Label.setBounds(x, 10, 60, 18); x += 62;
        param1Slider.setBounds(x, 8, 150, 22); x += 160;
    }

    muteButton.setBounds(x, 8, 28, 22); x += 32;
    soloButton.setBounds(x, 8, 28, 22); x += 40;

    inspectPatchButton.setBounds(x, 8, 110, 22);
}


TrackViewComponent::TrackViewComponent(RelativisticNodeGraph& graph)
    : nodeGraph(graph)
{
    addAndMakeVisible(addTrackButton);
    addTrackButton.onClick = [this]() {
        refreshTracks();
    };
    refreshTracks();
}

void TrackViewComponent::refreshTracks()
{
    trackLanes.clear();

    int trackIdx = 1;
    for (const auto& node : nodeGraph.getNodes())
    {
        if (node->getSymbol() == "msg" || node->getSymbol() == "out~") continue;

        auto lane = new TrackLaneComponent(trackIdx++, node->getLabel(), node);
        lane->onOpenPatchNode = [this](int nId) {
            if (onInspectNodePatch) onInspectNodePatch(nId);
        };
        addAndMakeVisible(lane);
        trackLanes.add(lane);
    }

    resized();
    repaint();
}

void TrackViewComponent::paint(juce::Graphics& g)
{
    g.fillAll(CarbonGoldLookAndFeel::carbonBg);
}

void TrackViewComponent::resized()
{
    int y = 10;
    int w = getWidth() - 20;

    addTrackButton.setBounds(10, y, 110, 26);
    y += 36;

    for (auto* lane : trackLanes)
    {
        lane->setBounds(10, y, w, 40);
        y += 46;
    }
}

} // namespace TimeDilationDAW
