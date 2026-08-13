#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/RelativisticNodeGraph.h"

namespace TimeDilationDAW
{

class RelativisticCanvasComponent : public juce::Component, private juce::Timer
{
public:
    RelativisticCanvasComponent(RelativisticNodeGraph& graph);
    ~RelativisticCanvasComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override
    {
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void pushSubGraphView(RelativisticNodeGraph* subGraph, const std::string& name);
    void popSubGraphView();
    RelativisticNodeGraph& getCurrentGraph();
    void spawnObjectEditorAt(juce::Point<float> pos);
    void spawnMessageBoxForNode(int targetNodeId, const std::string& msgText);
    void recenterView();

    std::function<void(std::shared_ptr<RelativisticNode>)> onNodeSelected;

private:
    RelativisticNodeGraph& rootGraph;
    std::vector<RelativisticNodeGraph*> graphStack;
    std::vector<std::string> breadcrumbs;

    int selectedNodeId = -1;
    int selectedConnectionId = -1;
    int draggingPortNodeId = -1;
    int draggingPortIdx = -1;
    bool isDraggingFromOutlet = true;
    juce::Point<float> dragCurrentPos;
    juce::Point<float> nodeDragStartPos;
    bool isResizingNode = false;
    juce::Point<float> nodeResizeStartSize;
    juce::Point<float> lastMousePos{ 150.0f, 150.0f };

    juce::TextEditor objectEditor;
    bool isEditingObject = false;
    int editingNodeId = -1;

    // View Panning & Recenter State
    float viewOffsetX = 0.0f;
    float viewOffsetY = 0.0f;
    bool isPanning = false;
    juce::Point<float> panStartMouse;
    juce::Point<float> panStartOffset;
    juce::TextButton recenterButton{ "Recenter View" };

    void spawnObjectEditorForNode(int nodeId);
    void commitObjectCreation();
    void cancelObjectCreation();

    juce::Rectangle<float> getNodeBounds(const RelativisticNode& node) const;
    juce::Point<float> getPortPos(const RelativisticNode& node, bool isOutlet, int portIdx) const;
};

} // namespace TimeDilationDAW
