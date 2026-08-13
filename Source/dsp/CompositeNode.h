#pragma once

#include "RelativisticNodeGraph.h"
#include <memory>
#include <string>

namespace TimeDilationDAW
{

class CompositeNode : public RelativisticNode
{
public:
    CompositeNode(int id, const std::string& patchSymbol = "patch~", const std::string& label = "patch~ synth");
    ~CompositeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;

    RelativisticNodeGraph& getInternalSubGraph() { return *subGraph; }
    const RelativisticNodeGraph& getInternalSubGraph() const { return *subGraph; }

    std::string saveToJSON() const;
    bool loadFromJSON(const std::string& jsonStr);

private:
    std::unique_ptr<RelativisticNodeGraph> subGraph;
    juce::AudioBuffer<float> subGraphMasterOut;

    void setupDefaultInternalPatch();
};

} // namespace TimeDilationDAW
