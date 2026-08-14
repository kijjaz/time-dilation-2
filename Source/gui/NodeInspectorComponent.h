#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/RelativisticNodeGraph.h"
#include "../dsp/RelativisticSoundNodes.h"

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
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    std::function<void(int nodeId, const std::string& msgText)> onSpawnMessageBox;

private:
    std::shared_ptr<RelativisticNode> selectedNode;

    juce::Label titleLabel{ "Inspector", "INSPECTOR" };
    juce::Label nodeTypeLabel{ "NodeType", "No Node Selected" };

    // Node Geometry (Position & Size) Controls
    juce::Slider posXSlider;
    juce::Label posXLabel{ "PosXLabel", "Position X (px)" };

    juce::Slider posYSlider;
    juce::Label posYLabel{ "PosYLabel", "Position Y (px)" };

    juce::Slider widthSlider;
    juce::Label widthLabel{ "WidthLabel", "Width (px)" };

    juce::Slider heightSlider;
    juce::Label heightLabel{ "HeightLabel", "Height (px)" };

    // Realtime Scope Settings
    juce::ToggleButton scopeVisibleToggle{ "Show Realtime Scope" };

    juce::ComboBox scopeTypeCombo;
    juce::Label scopeTypeLabel{ "ScopeTypeLabel", "Scope Display Variable" };

    juce::ComboBox scopeEngineCombo;
    juce::Label scopeEngineLabel{ "ScopeEngineLabel", "Scope Render Engine" };

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

    // Time Coupling Controls (Speed gamma vs Offset tau)
    juce::ComboBox timeCouplingCombo;
    juce::Label timeCouplingLabel;
    juce::Slider offsetCouplingSlider;
    juce::Label offsetCouplingLabel;

    // TidalCycles Pattern Text Field
    juce::Label tidalPatternLabel{ "TidalPatLabel", "Tidal Mini-Notation Pattern" };
    juce::TextEditor tidalPatternEditor;

    // Documentation & Method Reference Section
    juce::Label docTitleLabel{ "DocTitle", "DOCUMENTATION & METHODS" };
    juce::Label descLabel;
    juce::Label inletOutletLabel;

    std::vector<std::unique_ptr<juce::TextButton>> methodButtons;

    void updateUIForSelectedNode();
};

} // namespace TimeDilationDAW
