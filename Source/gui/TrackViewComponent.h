#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/RelativisticNodeGraph.h"

namespace TimeDilationDAW
{

class TrackLaneComponent : public juce::Component
{
public:
    TrackLaneComponent(int trackId, const std::string& trackName, std::shared_ptr<RelativisticNode> sourceNode);
    ~TrackLaneComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int nodeId)> onOpenPatchNode;
    void updateParamSliderFromNode(double val);

private:
    int trackId;
    std::string trackName;
    std::shared_ptr<RelativisticNode> sourceNode;

    juce::Label nameLabel;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;

    juce::Slider param1Slider;
    juce::Label param1Label;

    juce::TextButton muteButton{ "M" };
    juce::TextButton soloButton{ "S" };
    juce::TextButton inspectPatchButton{ "\U0001f50c Patch Node" };

    bool isMuted = false;
    bool isSoloed = false;
};

class TrackViewComponent : public juce::Component
{
public:
    TrackViewComponent(RelativisticNodeGraph& graph);
    ~TrackViewComponent() override = default;

    void refreshTracks();
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int nodeId)> onInspectNodePatch;

private:
    RelativisticNodeGraph& nodeGraph;
    juce::OwnedArray<TrackLaneComponent> trackLanes;
    juce::TextButton addTrackButton{ "+ Add Track" };
};

} // namespace TimeDilationDAW
