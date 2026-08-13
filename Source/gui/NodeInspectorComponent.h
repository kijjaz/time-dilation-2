#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/RelativisticNodeGraph.h"

namespace TimeDilationDAW
{

class NodeInspectorComponent : public juce::Component
{
public:
    NodeInspectorComponent();
    ~NodeInspectorComponent() override = default;

    void setSelectedNode(std::shared_ptr<RelativisticNode> node);
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int nodeId, const std::string& msgText)> onSpawnMessageBox;

private:
    std::shared_ptr<RelativisticNode> selectedNode;

    juce::Label titleLabel{ "Inspector", "INSPECTOR" };
    juce::Label nodeTypeLabel{ "NodeType", "No Node Selected" };

    // Per-Node Volume Gain Staging Slider
    juce::Slider volSlider;
    juce::Label volLabel{ "VolLabel", "Output Volume (Gain)" };

    // Parameter Controls
    juce::Slider paramSlider1;
    juce::Label paramLabel1;

    juce::Slider paramSlider2;
    juce::Label paramLabel2;

    juce::ComboBox optionSelector;
    juce::Label optionLabel;

    // Documentation & Method Reference Section
    juce::Label docTitleLabel{ "DocTitle", "DOCUMENTATION & METHODS" };
    juce::Label descLabel;
    juce::Label inletOutletLabel;

    std::vector<std::unique_ptr<juce::TextButton>> methodButtons;

    void updateUIForSelectedNode();
};

} // namespace TimeDilationDAW
