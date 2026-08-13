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

    // Multi-Selection Helper Methods
    bool isNodeSelected(int nodeId) const { return selectedNodeIds.find(nodeId) != selectedNodeIds.end(); }
    void selectNode(int nodeId) { selectedNodeIds.insert(nodeId); }
    void deselectNode(int nodeId) { selectedNodeIds.erase(nodeId); }
    void toggleNodeSelection(int nodeId)
    {
        if (isNodeSelected(nodeId)) deselectNode(nodeId);
        else selectNode(nodeId);
    }
    void clearSelection() { selectedNodeIds.clear(); selectedConnectionId = -1; }
    void selectAllNodes();

    // Clipboard & Editing Operations
    void copySelectedNodes();
    void cutSelectedNodes();
    void pasteClipboardNodes();
    void duplicateSelectedNodes();
    void deleteSelectedNodes();

    std::function<void(std::shared_ptr<RelativisticNode>)> onNodeSelected;

private:
    RelativisticNodeGraph& rootGraph;
    std::vector<RelativisticNodeGraph*> graphStack;
    std::vector<std::string> breadcrumbs;

    std::unordered_set<int> selectedNodeIds;
    int selectedConnectionId = -1;
    int draggingPortNodeId = -1;
    int draggingPortIdx = -1;
    bool isDraggingFromOutlet = true;
    juce::Point<float> dragCurrentPos;
    juce::Point<float> nodeDragStartPos;
    std::unordered_map<int, juce::Point<float>> multiNodeDragStarts;

    // Marquee / Lasso Selection Box State
    bool isMarqueeSelecting = false;
    juce::Point<float> marqueeStartPos;
    juce::Rectangle<float> marqueeRect;

    bool isResizingNode = false;
    juce::Point<float> nodeResizeStartSize;
    juce::Point<float> lastMousePos{ 150.0f, 150.0f };

    struct AutocompleteItem
    {
        std::string symbol;
        std::string description;
    };

    class AutocompleteModel : public juce::ListBoxModel
    {
    public:
        AutocompleteModel(RelativisticCanvasComponent& owner) : canvas(owner) {}
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    private:
        RelativisticCanvasComponent& canvas;
    };

    juce::TextEditor objectEditor;
    juce::ListBox suggestionListBox;
    AutocompleteModel autocompleteModel{ *this };
    std::vector<AutocompleteItem> allCatalogueObjects;
    std::vector<AutocompleteItem> filteredObjects;

    void updateAutocompleteSuggestions();
    void selectAutocompleteSuggestion(int row);

    bool isEditingObject = false;
    int editingNodeId = -1;

    // Canvas Node & Cable Clipboard
    struct ClipboardNodeData
    {
        int originalId = 0;
        std::string symbol;
        std::string label;
        float xPos = 0.0f;
        float yPos = 0.0f;
        float width = 120.0f;
        float height = 70.0f;
    };

    struct ClipboardConnectionData
    {
        int sourceNodeId = 0;
        int sourcePortIndex = 0;
        int destNodeId = 0;
        int destPortIndex = 0;
        PortDataType dataType = PortDataType::Audio;
    };

    struct CanvasClipboard
    {
        std::vector<ClipboardNodeData> nodes;
        std::vector<ClipboardConnectionData> connections;
        bool isEmpty() const { return nodes.empty(); }
    };

    CanvasClipboard clipboard;

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
