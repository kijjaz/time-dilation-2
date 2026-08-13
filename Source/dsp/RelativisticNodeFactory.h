#pragma once

#include "RelativisticNodeGraph.h"
#include "RelativisticSoundNodes.h"
#include "RelativisticSequencers.h"
#include "CompositeNode.h"
#include <memory>
#include <string>

namespace TimeDilationDAW
{

class RelativisticNodeFactory
{
public:
    static std::shared_ptr<RelativisticNode> createNode(int nodeId, const std::string& symbolAndArgs);
};

} // namespace TimeDilationDAW
