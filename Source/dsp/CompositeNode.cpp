#include "CompositeNode.h"
#include "RelativisticNodeFactory.h"
#include <iostream>

namespace TimeDilationDAW
{

CompositeNode::CompositeNode(int id, const std::string& patchSymbol, const std::string& label)
    : RelativisticNode(id, patchSymbol, label), subGraph(std::make_unique<RelativisticNodeGraph>())
{
    addInlet("timeIn", PortDataType::Time);
    addInlet("note", PortDataType::Audio);
    addOutlet("out~", PortDataType::Audio);

    subGraphMasterOut.setSize(2, 512);

    setupDefaultInternalPatch();
}

void CompositeNode::setupDefaultInternalPatch()
{
    subGraph->clearGraph();

    // Internal atomic sub-graph: osc~ -> ladder~ -> drive~ -> out~
    auto osc = RelativisticNodeFactory::createNode(1, "osc~ sin");
    osc->xPos = 100; osc->yPos = 100;

    auto ladder = RelativisticNodeFactory::createNode(2, "ladder~ 1200 0.5");
    ladder->xPos = 260; ladder->yPos = 100;

    auto driveNode = RelativisticNodeFactory::createNode(3, "drive~ 1.8");
    driveNode->xPos = 420; driveNode->yPos = 100;

    auto out = RelativisticNodeFactory::createNode(4, "out~");
    out->xPos = 580; out->yPos = 100;

    subGraph->addNode(osc);
    subGraph->addNode(ladder);
    subGraph->addNode(driveNode);
    subGraph->addNode(out);

    subGraph->addConnection(1, 0, 2, 0);
    subGraph->addConnection(2, 0, 3, 0);
    subGraph->addConnection(3, 0, 4, 0); // drive -> out~ in1~ (L)
    subGraph->addConnection(3, 0, 4, 1); // drive -> out~ in2~ (R)
}

void CompositeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    subGraphMasterOut.setSize(2, samplesPerBlock, false, true, true);
    subGraph->prepare(sampleRate, samplesPerBlock);
}

void CompositeNode::process(int numSamples)
{
    subGraphMasterOut.clear();

    // Map external inlet timeIn to internal sub-graph time frame
    const auto& timeInFrame = getInletTimeFrame(0);
    for (const auto& node : subGraph->getNodes())
    {
        if (node->getInlets().size() > 0 && node->getInlets()[0].dataType == PortDataType::Time)
        {
            node->setInletTimeFrameData(0, timeInFrame);
        }
    }

    // Process internal sub-graph
    subGraph->process(subGraphMasterOut, numSamples);

    // Map internal master output to composite node outlet
    auto& outBuf = getOutletBuffer(0);
    outBuf.copyFrom(0, 0, subGraphMasterOut, 0, 0, numSamples);
    outBuf.copyFrom(1, 0, subGraphMasterOut, 1, 0, numSamples);
}

std::string CompositeNode::saveToJSON() const
{
    return subGraph ? subGraph->serializeToJSON() : "{}";
}

bool CompositeNode::loadFromJSON(const std::string& jsonStr)
{
    return subGraph ? subGraph->deserializeFromJSON(jsonStr) : false;
}

} // namespace TimeDilationDAW
