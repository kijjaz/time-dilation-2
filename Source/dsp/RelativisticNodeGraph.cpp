#include "RelativisticNodeGraph.h"
#include "RelativisticNodeFactory.h"
#include <algorithm>
#include <iostream>

namespace TimeDilationDAW
{

// ============================================================================
// RelativisticNode Implementation
// ============================================================================

RelativisticNode::RelativisticNode(int id, const std::string& symbol, const std::string& label)
    : nodeId(id), symbolType(symbol), nodeLabel(label.empty() ? symbol : label)
{
    addInlet("msgIn", PortDataType::Message);
    addOutlet("msgOut", PortDataType::Message);
}

void RelativisticNode::addInlet(const std::string& name, PortDataType type)
{
    Port p;
    p.id = static_cast<int>(inlets.size());
    p.name = name;
    p.dataType = type;
    p.direction = PortDirection::Inlet;
    p.nodeOwnerId = nodeId;
    p.portIndex = static_cast<int>(inlets.size());
    inlets.push_back(p);

    inletBuffers.emplace_back(2, 512);
    inletTimeFrames.emplace_back();
}

void RelativisticNode::addOutlet(const std::string& name, PortDataType type)
{
    Port p;
    p.id = static_cast<int>(outlets.size());
    p.name = name;
    p.dataType = type;
    p.direction = PortDirection::Outlet;
    p.nodeOwnerId = nodeId;
    p.portIndex = static_cast<int>(outlets.size());
    outlets.push_back(p);

    outletBuffers.emplace_back(2, 512);
    outletTimeFrames.emplace_back();
}

void RelativisticNode::prepare(double sRate, int sPerBlock)
{
    currentSampleRate = sRate;
    currentBlockSize = sPerBlock;

    audioHistory.prepare(sRate, 10.0);

    for (auto& buf : inletBuffers)
    {
        buf.setSize(2, sPerBlock, false, true, true);
        buf.clear();
    }
    for (auto& buf : outletBuffers)
    {
        buf.setSize(2, sPerBlock, false, true, true);
        buf.clear();
    }
    for (auto& tf : inletTimeFrames)
    {
        tf.reset();
    }
    for (auto& tf : outletTimeFrames)
    {
        tf.reset();
    }
    audioScopeBuffer.assign(256, 0.0f);
    audioScopeWriteIdx = 0;
    timeScopeBuffer.assign(256, 0.0f);
    timeScopeWriteIdx = 0;
}

juce::AudioBuffer<float>& RelativisticNode::getOutletBuffer(int index)
{
    static juce::AudioBuffer<float> dummy(2, 512);
    if (index >= 0 && index < static_cast<int>(outletBuffers.size()))
        return outletBuffers[index];
    return dummy;
}

const juce::AudioBuffer<float>& RelativisticNode::getInletBuffer(int index) const
{
    static juce::AudioBuffer<float> dummy(2, 512);
    if (index >= 0 && index < static_cast<int>(inletBuffers.size()))
        return inletBuffers[index];
    return dummy;
}

int RelativisticNode::getInletIndex(const std::string& name) const
{
    for (size_t i = 0; i < inlets.size(); ++i)
    {
        if (inlets[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

int RelativisticNode::getOutletIndex(const std::string& name) const
{
    for (size_t i = 0; i < outlets.size(); ++i)
    {
        if (outlets[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

juce::AudioBuffer<float>& RelativisticNode::getAudioOutlet(const std::string& name)
{
    int idx = getOutletIndex(name);
    if (idx < 0)
    {
        std::cout << "[GetAudioOutletWarn] Node " << symbolType << " could not find outlet '" << name << "', defaulting to 0\n";
    }
    return getOutletBuffer(idx >= 0 ? idx : 0);
}

const juce::AudioBuffer<float>& RelativisticNode::getAudioInlet(const std::string& name) const
{
    int idx = getInletIndex(name);
    return getInletBuffer(idx >= 0 ? idx : 1);
}

void RelativisticNode::receiveMessage(const std::string& message)
{
    juce::String msgStr(message);
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.isEmpty()) return;

    // Log control event to internal proper-time pipe
    double curTau = !inletTimeFrames.empty() ? inletTimeFrames[0].masterTau : 0.0;
    controlPipe.addEvent(curTau, message);

    if (tokens.size() >= 2 && (tokens[0] == "vol" || tokens[0] == "volume"))
    {
        juce::String valStr = tokens[1].toLowerCase();
        if (valStr == "-inf" || valStr == "inf" || valStr == "mute")
        {
            setVolumeDb(-100.0f);
        }
        else
        {
            float val = valStr.getFloatValue();
            // If negative or explicitly <= 6.0f, interpret as dBFS logarithmic scale
            if (valStr.endsWith("db") || val <= 6.0f)
            {
                setVolumeDb(val);
            }
            else
            {
                setOutputVolume(val);
            }
        }
    }
    else if (tokens.size() >= 2 && (tokens[0] == "time_mode" || tokens[0] == "mode"))
    {
        juce::String modeStr = tokens[1].toLowerCase();
        if (modeStr == "both" || modeStr == "full" || modeStr == "all")
        {
            setTimeCouplingMode(TimeCouplingMode::Both);
        }
        else if (modeStr == "speed" || modeStr == "speedonly" || modeStr == "gamma" || modeStr == "dilation")
        {
            setTimeCouplingMode(TimeCouplingMode::SpeedOnly);
        }
        else if (modeStr == "offset" || modeStr == "offsetonly" || modeStr == "tau" || modeStr == "position")
        {
            setTimeCouplingMode(TimeCouplingMode::OffsetOnly);
        }
        else if (modeStr == "none" || modeStr == "bypass" || modeStr == "off")
        {
            setTimeCouplingMode(TimeCouplingMode::Bypassed);
        }
    }
    else if (tokens.size() >= 2 && (tokens[0] == "coupling" || tokens[0] == "time_coupling" || tokens[0] == "offset_coupling"))
    {
        setOffsetCouplingFactor(tokens[1].getDoubleValue());
    }
}

TimePolyFrame& RelativisticNode::getTimeOutlet(const std::string& name)
{
    int idx = getOutletIndex(name);
    return getOutletTimeFrame(idx >= 0 ? idx : 1);
}

const TimePolyFrame& RelativisticNode::getTimeInlet(const std::string& name) const
{
    int idx = getInletIndex(name);
    return getInletTimeFrame(idx >= 0 ? idx : 1);
}

TimePolyFrame& RelativisticNode::getOutletTimeFrame(int index)
{
    static TimePolyFrame dummy;
    if (index >= 0 && index < static_cast<int>(outletTimeFrames.size()))
        return outletTimeFrames[index];
    return dummy;
}

const TimePolyFrame& RelativisticNode::getInletTimeFrame(int index) const
{
    static TimePolyFrame dummy;
    if (index >= 0 && index < static_cast<int>(inletTimeFrames.size()))
        return inletTimeFrames[index];
    return dummy;
}

void RelativisticNode::setInletBufferData(int portIndex, const juce::AudioBuffer<float>& data)
{
    if (portIndex >= 0 && portIndex < static_cast<int>(inletBuffers.size()))
    {
        int numChans = std::min(data.getNumChannels(), inletBuffers[portIndex].getNumChannels());
        int numSamps = std::min(data.getNumSamples(), inletBuffers[portIndex].getNumSamples());
        for (int ch = 0; ch < numChans; ++ch)
        {
            inletBuffers[portIndex].addFrom(ch, 0, data, ch, 0, numSamps);
        }
    }
}

void RelativisticNode::setInletTimeFrameData(int portIndex, const TimePolyFrame& frame)
{
    if (portIndex >= 0 && portIndex < static_cast<int>(inletTimeFrames.size()))
    {
        inletTimeFrames[portIndex] = frame;
    }
}


// ============================================================================
// RelativisticNodeGraph Implementation
// ============================================================================

RelativisticNodeGraph::RelativisticNodeGraph()
{
}

void RelativisticNodeGraph::prepare(double sRate, int sPerBlock)
{
    sampleRate = sRate;
    samplesPerBlock = sPerBlock;

    dcR = std::clamp(static_cast<float>(1.0 - (2.0 * 3.14159265358979323846 * 5.0 / sRate)), 0.99f, 0.99999f);
    feedbackScratchBuffer.setSize(2, sPerBlock, false, true, true);
    feedbackScratchBuffer.clear();

    preCausalBuffer.prepare(sRate, 10.0);

    for (auto& node : nodes)
    {
        node->prepare(sRate, sPerBlock);
    }

    // Allocate 1-block delay feedback buffers for each node outlet
    previousBlockBuffers.clear();
    previousBlockTimeFrames.clear();

    for (auto& node : nodes)
    {
        int nId = node->getId();
        int numOutlets = static_cast<int>(node->getOutlets().size());
        previousBlockBuffers[nId].resize(numOutlets);
        previousBlockTimeFrames[nId].resize(numOutlets);

        for (int o = 0; o < numOutlets; ++o)
        {
            previousBlockBuffers[nId][o].setSize(2, sPerBlock, false, true, true);
            previousBlockBuffers[nId][o].clear();
            previousBlockTimeFrames[nId][o].reset();
        }
    }

    updateTopologicalSort();
}

void RelativisticNodeGraph::setConnectionFeedbackProtection(int connectionId, bool softClip, bool dcBlock)
{
    for (auto& c : connections)
    {
        if (c.connectionId == connectionId)
        {
            c.enableSoftClip = softClip;
            c.enableDcBlock = dcBlock;
            break;
        }
    }
}

int RelativisticNodeGraph::addNode(std::shared_ptr<RelativisticNode> node)
{
    if (!node) return -1;
    int id = node->getId();
    nodes.push_back(node);
    nodeMap[id] = node;
    node->prepare(sampleRate, samplesPerBlock);

    // Allocate 1-block delay feedback buffers for new node's outlets
    int numOutlets = static_cast<int>(node->getOutlets().size());
    previousBlockBuffers[id].resize(numOutlets);
    previousBlockTimeFrames[id].resize(numOutlets);

    int blockSize = samplesPerBlock > 0 ? samplesPerBlock : 512;
    for (int o = 0; o < numOutlets; ++o)
    {
        previousBlockBuffers[id][o].setSize(2, blockSize, false, true, true);
        previousBlockBuffers[id][o].clear();
        previousBlockTimeFrames[id][o].reset();
    }

    bool isTimeNode = (node->getSymbol().rfind("time.", 0) == 0 || node->getSymbol() == "seq");
    node->displayType = isTimeNode ? RelativisticNode::ScopeDisplayType::TimeFrame : RelativisticNode::ScopeDisplayType::AudioWaveform;

    node->onOutletMessageEmitted = [this, id](int outletIdx, const std::string& msgText) mutable {
        double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        for (auto& conn : connections)
        {
            if (conn.sourceNodeId == id && conn.sourcePortIndex == outletIdx)
            {
                conn.lastMessageTriggerTime = now;
                auto destNode = getNode(conn.destNodeId);
                if (destNode)
                {
                    destNode->receiveMessage(msgText);
                }
            }
        }
    };

    updateTopologicalSort();
    return id;
}

bool RelativisticNodeGraph::removeNode(int nodeId)
{
    auto it = std::remove_if(nodes.begin(), nodes.end(), [nodeId](const std::shared_ptr<RelativisticNode>& n) {
        return n->getId() == nodeId;
    });
    if (it != nodes.end())
    {
        nodes.erase(it, nodes.end());
        nodeMap.erase(nodeId);

        // Erase connections
        connections.erase(std::remove_if(connections.begin(), connections.end(), [nodeId](const PatchConnection& c) {
            return c.sourceNodeId == nodeId || c.destNodeId == nodeId;
        }), connections.end());

        updateTopologicalSort();
        return true;
    }
    return false;
}

void RelativisticNodeGraph::clearGraph()
{
    nodes.clear();
    nodeMap.clear();
    connections.clear();
    sortedNodes.clear();
    previousBlockBuffers.clear();
    previousBlockTimeFrames.clear();
}

bool RelativisticNodeGraph::addConnection(int srcNodeId, int srcPortIdx, int destNodeId, int destPortIdx)
{
    auto srcNode = getNode(srcNodeId);
    auto destNode = getNode(destNodeId);
    if (!srcNode || !destNode) return false;

    if (srcPortIdx < 0 || srcPortIdx >= static_cast<int>(srcNode->getOutlets().size())) return false;
    if (destPortIdx < 0 || destPortIdx >= static_cast<int>(destNode->getInlets().size())) return false;

    PatchConnection conn;
    conn.connectionId = nextConnectionId++;
    conn.sourceNodeId = srcNodeId;
    conn.sourcePortIndex = srcPortIdx;
    conn.destNodeId = destNodeId;
    conn.destPortIndex = destPortIdx;
    conn.dataType = srcNode->getOutlets()[srcPortIdx].dataType;

    connections.push_back(conn);
    updateTopologicalSort();
    return true;
}

bool RelativisticNodeGraph::removeConnection(int connectionId)
{
    auto it = std::remove_if(connections.begin(), connections.end(), [connectionId](const PatchConnection& c) {
        return c.connectionId == connectionId;
    });
    if (it != connections.end())
    {
        connections.erase(it, connections.end());
        updateTopologicalSort();
        return true;
    }
    return false;
}

std::shared_ptr<RelativisticNode> RelativisticNodeGraph::getNode(int nodeId)
{
    auto it = nodeMap.find(nodeId);
    if (it != nodeMap.end()) return it->second;
    return nullptr;
}

void RelativisticNodeGraph::runTarjanSCC()
{
    // Mark cycle feedback paths using Tarjan's SCC algorithm
    for (auto& c : connections)
    {
        c.isFeedbackCycle = false;
    }

    // Map adjacency
    std::unordered_map<int, std::vector<int>> adj;
    for (const auto& c : connections)
    {
        adj[c.sourceNodeId].push_back(c.destNodeId);
    }

    int index = 0;
    std::unordered_map<int, int> nodeIndices;
    std::unordered_map<int, int> nodeLowLink;
    std::unordered_set<int> onStack;
    std::stack<int> sccStack;
    std::vector<std::vector<int>> sccs;

    std::function<void(int)> strongConnect = [&](int v) {
        nodeIndices[v] = index;
        nodeLowLink[v] = index;
        index++;
        sccStack.push(v);
        onStack.insert(v);

        if (adj.count(v))
        {
            for (int w : adj[v])
            {
                if (nodeIndices.find(w) == nodeIndices.end())
                {
                    strongConnect(w);
                    nodeLowLink[v] = std::min(nodeLowLink[v], nodeLowLink[w]);
                }
                else if (onStack.count(w))
                {
                    nodeLowLink[v] = std::min(nodeLowLink[v], nodeIndices[w]);
                }
            }
        }

        if (nodeLowLink[v] == nodeIndices[v])
        {
            std::vector<int> scc;
            while (true)
            {
                int w = sccStack.top();
                sccStack.pop();
                onStack.erase(w);
                scc.push_back(w);
                if (w == v) break;
            }
            if (scc.size() > 1)
            {
                sccs.push_back(scc);
            }
        }
    };

    for (const auto& n : nodes)
    {
        int id = n->getId();
        if (nodeIndices.find(id) == nodeIndices.end())
        {
            strongConnect(id);
        }
    }

    // Flag connections within SCC as feedback cycle connections, including single-node self-loops
    for (auto& c : connections)
    {
        if (c.sourceNodeId == c.destNodeId)
        {
            c.isFeedbackCycle = true;
            continue;
        }

        for (const auto& scc : sccs)
        {
            std::unordered_set<int> sccSet(scc.begin(), scc.end());
            if (sccSet.count(c.sourceNodeId) && sccSet.count(c.destNodeId))
            {
                c.isFeedbackCycle = true;
                break;
            }
        }
    }
}

void RelativisticNodeGraph::updateTopologicalSort()
{
    runTarjanSCC();

    // Kahn's algorithm for topological sorting non-feedback edges
    std::unordered_map<int, int> inDegree;
    std::unordered_map<int, std::vector<int>> adj;

    for (const auto& n : nodes)
    {
        inDegree[n->getId()] = 0;
    }

    for (const auto& c : connections)
    {
        if (!c.isFeedbackCycle)
        {
            adj[c.sourceNodeId].push_back(c.destNodeId);
            inDegree[c.destNodeId]++;
        }
    }

    std::vector<int> q;
    for (const auto& pair : inDegree)
    {
        if (pair.second == 0)
        {
            q.push_back(pair.first);
        }
    }

    sortedNodes.clear();
    while (!q.empty())
    {
        int curr = q.back();
        q.pop_back();

        auto n = getNode(curr);
        if (n) sortedNodes.push_back(n);

        if (adj.count(curr))
        {
            for (int neighbor : adj[curr])
            {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0)
                {
                    q.push_back(neighbor);
                }
            }
        }
    }

    // Append any remaining unvisited nodes
    for (const auto& n : nodes)
    {
        if (std::find(sortedNodes.begin(), sortedNodes.end(), n) == sortedNodes.end())
        {
            sortedNodes.push_back(n);
        }
    }
}

void RelativisticNodeGraph::process(juce::AudioBuffer<float>& masterOutBuffer, int numSamples)
{
    masterOutBuffer.clear();

    TimePolyFrame defaultFrame;
    defaultFrame.masterGamma = 1.0;

    // 1. Clear inlet buffers & reset default inlet time frames for all nodes
    for (auto& node : nodes)
    {
        for (int i = 0; i < static_cast<int>(node->getInlets().size()); ++i)
        {
            const_cast<juce::AudioBuffer<float>&>(node->getInletBuffer(i)).clear();
            node->setInletTimeFrameData(i, defaultFrame);
        }
    }

    // 2. Transfer feedback cycle connection data using 1-block history
    for (auto& conn : connections)
    {
        auto destNode = getNode(conn.destNodeId);
        if (!destNode || !conn.isFeedbackCycle) continue;

        if (conn.dataType == PortDataType::Audio)
        {
            if (previousBlockBuffers.count(conn.sourceNodeId) &&
                conn.sourcePortIndex < static_cast<int>(previousBlockBuffers[conn.sourceNodeId].size()))
            {
                const auto& srcBuf = previousBlockBuffers[conn.sourceNodeId][conn.sourcePortIndex];

                // If soft-clipper or DC blocker is enabled globally and for this connection, filter audio to prevent explosion
                if ((feedbackSoftClipEnabled || feedbackDcBlockEnabled) && (conn.enableSoftClip || conn.enableDcBlock))
                {
                    int numChans = std::min(2, srcBuf.getNumChannels());
                    feedbackScratchBuffer.setSize(numChans, numSamples, false, false, true);

                    const bool applyDc = feedbackDcBlockEnabled && conn.enableDcBlock;
                    const bool applyClip = feedbackSoftClipEnabled && conn.enableSoftClip;

                    for (int ch = 0; ch < numChans; ++ch)
                    {
                        const float* rPtr = srcBuf.getReadPointer(ch);
                        float* wPtr = feedbackScratchBuffer.getWritePointer(ch);
                        float x1 = conn.dcX1[ch];
                        float y1 = conn.dcY1[ch];

                        for (int s = 0; s < numSamples; ++s)
                        {
                            float x = rPtr[s];
                            // Sanitize against NaNs and Infs
                            if (std::isnan(x) || std::isinf(x)) x = 0.0f;

                            if (applyDc)
                            {
                                float y = x - x1 + dcR * y1;
                                x1 = x;
                                if (std::abs(y) < 1.0e-15f) y = 0.0f; // Denormal protection
                                y1 = y;
                                x = y;
                            }

                            if (applyClip)
                            {
                                x = std::tanh(x);
                            }

                            wPtr[s] = x;
                        }
                        conn.dcX1[ch] = x1;
                        conn.dcY1[ch] = y1;
                    }
                    destNode->setInletBufferData(conn.destPortIndex, feedbackScratchBuffer);
                }
                else
                {
                    destNode->setInletBufferData(conn.destPortIndex, srcBuf);
                }
            }
        }
        else // Time
        {
            if (previousBlockTimeFrames.count(conn.sourceNodeId) &&
                conn.sourcePortIndex < static_cast<int>(previousBlockTimeFrames[conn.sourceNodeId].size()))
            {
                destNode->setInletTimeFrameData(conn.destPortIndex, previousBlockTimeFrames[conn.sourceNodeId][conn.sourcePortIndex]);
            }
        }
    }

    // 3. Process sorted nodes in topological order and instantly push outputs downstream
    for (auto& node : sortedNodes)
    {
        node->process(numSamples);

        // Apply per-node output volume gain staging to audio outlets
        float vol = node->getOutputVolume();
        if (std::abs(vol - 1.0f) > 0.001f)
        {
            for (size_t i = 0; i < node->getOutlets().size(); ++i)
            {
                if (node->getOutlets()[i].dataType == PortDataType::Audio)
                {
                    node->getOutletBuffer(static_cast<int>(i)).applyGain(vol);
                }
            }
        }

        // Record 10-second audio ring history memory for relativistic time scrubbing/stretching
        int numOuts = static_cast<int>(node->getOutlets().size());
        int mainAudioOutlet = 0;
        for (size_t i = 0; i < node->getOutlets().size(); ++i)
        {
            if (node->getOutlets()[i].dataType == PortDataType::Audio)
            {
                mainAudioOutlet = static_cast<int>(i);
                break;
            }
        }
        if (mainAudioOutlet < numOuts)
        {
            node->audioHistory.writeBlock(node->getOutletBuffer(mainAudioOutlet), numSamples);
        }

        // 1. Record Audio Scope History & Audio Telemetry (RMS & Peak)
        for (size_t i = 0; i < node->getOutlets().size(); ++i)
        {
            if (node->getOutlets()[i].dataType == PortDataType::Audio)
            {
                const auto& buf = node->getOutletBuffer(static_cast<int>(i));
                if (buf.getNumChannels() > 0)
                {
                    node->updateAudioTelemetry(buf, numSamples);
                    const float* ptr = buf.getReadPointer(0);
                    for (int s = 0; s < numSamples; ++s)
                    {
                        node->pushAudioScopeSample(ptr[s]);
                    }
                }
                break;
            }
        }

        // 2. Record Proper Time Telemetry History (gamma, tau, coupling)
        for (size_t i = 0; i < node->getOutlets().size(); ++i)
        {
            if (node->getOutlets()[i].dataType == PortDataType::Time)
            {
                // Nodes like time.lfo push sample-accurate LFO waveforms during process()
                std::string sym = node->getSymbol();
                if (sym != "time.lfo" && sym != "time.lfo~")
                {
                    const auto& tf = node->getOutletTimeFrame(static_cast<int>(i));
                    float gammaVal = static_cast<float>(tf.masterGamma);
                    float tauVal = static_cast<float>(!tf.streams.empty() ? tf.streams[0].tau : 0.0);
                    float cVal = static_cast<float>(!tf.streams.empty() ? tf.streams[0].offsetCoupling : 1.0);

                    float sampleVal = gammaVal;
                    if (node->timeVarMode == RelativisticNode::TimeScopeVariable::OffsetTau)
                    {
                        sampleVal = tauVal;
                    }
                    else if (node->timeVarMode == RelativisticNode::TimeScopeVariable::CouplingC)
                    {
                        sampleVal = cVal;
                    }

                    for (int s = 0; s < numSamples; ++s)
                    {
                        node->pushTimeScopeSample(sampleVal);
                        node->pushTimeTauScopeSample(tauVal);
                    }
                }
                break;
            }
        }

        // Instantaneous transfer to non-feedback downstream inlets
        for (const auto& conn : connections)
        {
            if (conn.sourceNodeId == node->getId() && !conn.isFeedbackCycle)
            {
                auto destNode = getNode(conn.destNodeId);
                if (destNode)
                {
                    if (conn.dataType == PortDataType::Audio)
                    {
                        destNode->setInletBufferData(conn.destPortIndex, node->getOutletBuffer(conn.sourcePortIndex));
                    }
                    else // Time
                    {
                        destNode->setInletTimeFrameData(conn.destPortIndex, node->getOutletTimeFrame(conn.sourcePortIndex));
                    }
                }
            }
        }

        // Sum `out~` master inlets into masterOutBuffer (Inlet 1 -> Left, Inlet 2 -> Right)
        if (node->getSymbol() == "out~")
        {
            const auto& bufL = node->getInletBuffer(1); // in1~ (Audio L, Inlet 1)
            const auto& bufR = node->getInletBuffer(2); // in2~ (Audio R, Inlet 2)

            int chans = masterOutBuffer.getNumChannels();
            float outVol = node->getOutputVolume();

            if (chans > 0 && bufL.getNumChannels() > 0)
            {
                masterOutBuffer.addFrom(0, 0, bufL, 0, 0, numSamples, outVol);
            }
            if (chans > 1 && bufR.getNumChannels() > 0)
            {
                masterOutBuffer.addFrom(1, 0, bufR, 0, 0, numSamples, outVol);
            }
        }
    }

    // Evaluate maximum future look-ahead demand across all active nodes
    double maxFutureOffsetSec = 0.0053;
    if (manualDemandSec >= 0.0)
    {
        maxFutureOffsetSec = std::max(0.0053, manualDemandSec);
    }
    else
    {
        for (const auto& node : nodes)
        {
            if (node->getSymbol() == "time.lfo~")
            {
                maxFutureOffsetSec = std::max(maxFutureOffsetSec, 0.5);
            }
            else if (node->getSymbol() == "time.warp" || node->getSymbol() == "time.warp~")
            {
                const auto& frame = node->getOutletTimeFrame(0);
                if (frame.masterGamma > 1.5) maxFutureOffsetSec = std::max(maxFutureOffsetSec, 1.2);
            }
        }
    }
    latencyEngine.updateDemand(maxFutureOffsetSec);

    // Record raw synthesized block into pre-causal future look-ahead buffer
    int writeStartPos = preCausalBuffer.getWritePos();
    preCausalBuffer.writeBlock(masterOutBuffer, numSamples);

    // Read back smooth pre-causal audio sample-by-sample using C2 Hermite ramp & Hermite cubic sample interpolation
    int numChans = masterOutBuffer.getNumChannels();
    for (int s = 0; s < numSamples; ++s)
    {
        double latS = latencyEngine.advanceSampleSmooth();
        double sampleWritePos = static_cast<double>(writeStartPos + s);
        for (int ch = 0; ch < numChans; ++ch)
        {
            float val = preCausalBuffer.readFutureSampleHermiteAtPos(ch, sampleWritePos - latS);
            masterOutBuffer.setSample(ch, s, val);
        }
    }

    // 4. Update 1-block history buffers for next block cycle
    for (auto& node : nodes)
    {
        int nId = node->getId();
        int numOutlets = static_cast<int>(node->getOutlets().size());

        if (previousBlockBuffers.count(nId) && previousBlockTimeFrames.count(nId))
        {
            auto& bufVec = previousBlockBuffers[nId];
            auto& frameVec = previousBlockTimeFrames[nId];

            if (static_cast<int>(bufVec.size()) < numOutlets) bufVec.resize(numOutlets);
            if (static_cast<int>(frameVec.size()) < numOutlets) frameVec.resize(numOutlets);

            for (int o = 0; o < numOutlets; ++o)
            {
                if (bufVec[o].getNumSamples() != numSamples)
                {
                    bufVec[o].setSize(2, numSamples, false, true, true);
                }
                bufVec[o].makeCopyOf(node->getOutletBuffer(o));
                frameVec[o] = node->getOutletTimeFrame(o);
            }
        }
    }
}

std::string RelativisticNodeGraph::serializeToJSON() const
{
    juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();

    juce::Array<juce::var> nodesArr;
    for (const auto& node : nodes)
    {
        juce::DynamicObject::Ptr nObj = new juce::DynamicObject();
        nObj->setProperty("id", node->getId());
        nObj->setProperty("symbol", juce::String(node->getSymbol()));
        nObj->setProperty("label", juce::String(node->getLabel()));
        nObj->setProperty("xPos", node->xPos);
        nObj->setProperty("yPos", node->yPos);
        nObj->setProperty("width", node->width);
        nObj->setProperty("height", node->height);
        nObj->setProperty("outputVolume", node->outputVolume);
        nObj->setProperty("showRealtimeDisplay", node->showRealtimeDisplay);
        nObj->setProperty("displayType", static_cast<int>(node->displayType));
        nObj->setProperty("timeVarMode", static_cast<int>(node->timeVarMode));
        nObj->setProperty("scopeMode", static_cast<int>(node->scopeMode));
        nObj->setProperty("timeCouplingMode", static_cast<int>(node->timeCouplingMode));
        nObj->setProperty("offsetCouplingFactor", node->offsetCouplingFactor);
        nodesArr.add(juce::var(nObj));
    }
    rootObj->setProperty("nodes", nodesArr);

    rootObj->setProperty("feedbackSoftClipEnabled", feedbackSoftClipEnabled);
    rootObj->setProperty("feedbackDcBlockEnabled", feedbackDcBlockEnabled);

    juce::Array<juce::var> connsArr;
    for (const auto& conn : connections)
    {
        juce::DynamicObject::Ptr cObj = new juce::DynamicObject();
        cObj->setProperty("id", conn.connectionId);
        cObj->setProperty("srcNode", conn.sourceNodeId);
        cObj->setProperty("srcPort", conn.sourcePortIndex);
        cObj->setProperty("destNode", conn.destNodeId);
        cObj->setProperty("destPort", conn.destPortIndex);
        cObj->setProperty("enableSoftClip", conn.enableSoftClip);
        cObj->setProperty("enableDcBlock", conn.enableDcBlock);
        connsArr.add(juce::var(cObj));
    }
    rootObj->setProperty("connections", connsArr);

    return juce::JSON::toString(juce::var(rootObj)).toStdString();
}

bool RelativisticNodeGraph::deserializeFromJSON(const std::string& jsonStr)
{
    auto parsed = juce::JSON::parse(jsonStr);
    if (!parsed.isObject()) return false;

    clearGraph();
    auto rootObj = parsed.getDynamicObject();
    if (!rootObj) return false;

    if (rootObj->hasProperty("feedbackSoftClipEnabled"))
        feedbackSoftClipEnabled = rootObj->getProperty("feedbackSoftClipEnabled");
    if (rootObj->hasProperty("feedbackDcBlockEnabled"))
        feedbackDcBlockEnabled = rootObj->getProperty("feedbackDcBlockEnabled");

    auto nodesVar = rootObj->getProperty("nodes");
    if (nodesVar.isArray())
    {
        auto* arr = nodesVar.getArray();
        for (const auto& nVar : *arr)
        {
            auto* nObj = nVar.getDynamicObject();
            if (nObj)
            {
                int id = nObj->getProperty("id");
                std::string symbol = nObj->getProperty("symbol").toString().toStdString();
                std::string label = nObj->getProperty("label").toString().toStdString();
                float x = static_cast<float>(nObj->getProperty("xPos"));
                float y = static_cast<float>(nObj->getProperty("yPos"));

                auto node = RelativisticNodeFactory::createNode(id, symbol);
                if (node)
                {
                    node->setLabel(label);
                    node->xPos = x;
                    node->yPos = y;
                    if (nObj->hasProperty("width")) node->width = static_cast<float>(nObj->getProperty("width"));
                    if (nObj->hasProperty("height")) node->height = static_cast<float>(nObj->getProperty("height"));
                    if (nObj->hasProperty("outputVolume")) node->outputVolume = static_cast<float>(nObj->getProperty("outputVolume"));
                    if (nObj->hasProperty("showRealtimeDisplay")) node->showRealtimeDisplay = nObj->getProperty("showRealtimeDisplay");
                    if (nObj->hasProperty("displayType")) node->displayType = static_cast<RelativisticNode::ScopeDisplayType>(static_cast<int>(nObj->getProperty("displayType")));
                    if (nObj->hasProperty("timeVarMode")) node->timeVarMode = static_cast<RelativisticNode::TimeScopeVariable>(static_cast<int>(nObj->getProperty("timeVarMode")));
                    if (nObj->hasProperty("scopeMode")) node->scopeMode = static_cast<RelativisticNode::ScopeRenderMode>(static_cast<int>(nObj->getProperty("scopeMode")));
                    if (nObj->hasProperty("timeCouplingMode")) node->timeCouplingMode = static_cast<RelativisticNode::TimeCouplingMode>(static_cast<int>(nObj->getProperty("timeCouplingMode")));
                    if (nObj->hasProperty("offsetCouplingFactor")) node->offsetCouplingFactor = static_cast<double>(nObj->getProperty("offsetCouplingFactor"));

                    addNode(node);
                }
            }
        }
    }

    auto connsVar = rootObj->getProperty("connections");
    if (connsVar.isArray())
    {
        auto* arr = connsVar.getArray();
        for (const auto& cVar : *arr)
        {
            auto* cObj = cVar.getDynamicObject();
            if (cObj)
            {
                int srcNode = cObj->getProperty("srcNode");
                int srcPort = cObj->getProperty("srcPort");
                int destNode = cObj->getProperty("destNode");
                int destPort = cObj->getProperty("destPort");

                addConnection(srcNode, srcPort, destNode, destPort);

                if (!connections.empty())
                {
                    auto& lastConn = connections.back();
                    if (cObj->hasProperty("enableSoftClip"))
                        lastConn.enableSoftClip = cObj->getProperty("enableSoftClip");
                    if (cObj->hasProperty("enableDcBlock"))
                        lastConn.enableDcBlock = cObj->getProperty("enableDcBlock");
                }
            }
        }
    }

    updateTopologicalSort();
    return true;
}

} // namespace TimeDilationDAW
