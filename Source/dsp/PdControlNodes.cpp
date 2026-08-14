#include "PdControlNodes.h"
#include <algorithm>
#include <iostream>

namespace TimeDilationDAW
{

// ============================================================================
// TriggerNode ([trigger], [t])
// ============================================================================

TriggerNode::TriggerNode(int id, const std::vector<std::string>& types)
    : RelativisticNode(id, "trigger", "t"), outletTypes(types)
{
    if (outletTypes.empty())
    {
        outletTypes = { "b", "b" };
    }

    if (!outlets.empty())
    {
        outlets[0].name = "out0";
    }

    std::string lbl = "t";
    for (size_t i = 0; i < outletTypes.size(); ++i)
    {
        lbl += " " + outletTypes[i];
        if (i > 0)
        {
            addOutlet("out" + std::to_string(i), PortDataType::Message);
        }
    }
    setLabel(lbl);
}

void TriggerNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TriggerNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void TriggerNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    executeTrigger(message);
}

void TriggerNode::executeTrigger(const std::string& incomingVal)
{
    // Right-to-Left execution order: iterate from (N-1) down to 0
    int numOuts = static_cast<int>(outletTypes.size());
    for (int i = numOuts - 1; i >= 0; --i)
    {
        const std::string& type = outletTypes[static_cast<size_t>(i)];

        if (type == "b" || type == "bang")
        {
            emitMessageOnOutlet(i, "bang");
        }
        else if (type == "f" || type == "float")
        {
            // If incoming string contains a number, send it; otherwise send 0
            try
            {
                double val = std::stod(incomingVal);
                emitMessageOnOutlet(i, std::to_string(val));
            }
            catch (...)
            {
                emitMessageOnOutlet(i, "0");
            }
        }
        else if (type == "s" || type == "symbol")
        {
            emitMessageOnOutlet(i, incomingVal);
        }
        else if (type == "a" || type == "anything" || type == "list")
        {
            emitMessageOnOutlet(i, incomingVal);
        }
        else
        {
            // Constant value literal (e.g. "0", "1", "stop")
            emitMessageOnOutlet(i, type);
        }
    }
}


// ============================================================================
// SelectNode ([select], [sel])
// ============================================================================

SelectNode::SelectNode(int id, const std::vector<std::string>& targets)
    : RelativisticNode(id, "select", "sel"), matchTargets(targets)
{
    if (matchTargets.empty())
    {
        matchTargets = { "0" };
    }

    if (!outlets.empty())
    {
        outlets[0].name = "match0";
    }

    std::string lbl = "sel";
    for (size_t i = 0; i < matchTargets.size(); ++i)
    {
        lbl += " " + matchTargets[i];
        if (i > 0)
        {
            addOutlet("match" + std::to_string(i), PortDataType::Message);
        }
    }
    addOutlet("unmatched", PortDataType::Message); // Rightmost pass-through outlet
    setLabel(lbl);
}

void SelectNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void SelectNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void SelectNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    testMatch(message);
}

void SelectNode::testMatch(const std::string& incomingVal)
{
    // Trim whitespace
    juce::String cleanVal = juce::String(incomingVal).trim();
    int matchIdx = -1;

    for (size_t i = 0; i < matchTargets.size(); ++i)
    {
        juce::String target(matchTargets[i]);
        if (cleanVal == target)
        {
            matchIdx = static_cast<int>(i);
            break;
        }

        // Try numeric comparison if both are floating-point numbers
        try
        {
            double inNum = cleanVal.getDoubleValue();
            double tgtNum = target.getDoubleValue();
            if (std::abs(inNum - tgtNum) < 1.0e-9)
            {
                matchIdx = static_cast<int>(i);
                break;
            }
        }
        catch (...) {}
    }

    if (matchIdx >= 0)
    {
        emitMessageOnOutlet(matchIdx, "bang");
    }
    else
    {
        // Emit unmatched value out rightmost outlet
        int rightmostOutlet = static_cast<int>(matchTargets.size());
        emitMessageOnOutlet(rightmostOutlet, cleanVal.toStdString());
    }
}


// ============================================================================
// RouteNode ([route])
// ============================================================================

RouteNode::RouteNode(int id, const std::vector<std::string>& selectors)
    : RelativisticNode(id, "route", "route"), selectorKeys(selectors)
{
    if (selectorKeys.empty())
    {
        selectorKeys = { "pitch" };
    }

    if (!outlets.empty())
    {
        outlets[0].name = "route0";
    }

    std::string lbl = "route";
    for (size_t i = 0; i < selectorKeys.size(); ++i)
    {
        lbl += " " + selectorKeys[i];
        if (i > 0)
        {
            addOutlet("route" + std::to_string(i), PortDataType::Message);
        }
    }
    addOutlet("unmatched", PortDataType::Message);
    setLabel(lbl);
}

void RouteNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void RouteNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void RouteNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    executeRoute(message);
}

void RouteNode::executeRoute(const std::string& incomingMsg)
{
    juce::String msgStr = juce::String(incomingMsg).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.isEmpty()) return;

    juce::String leadToken = tokens[0];
    int matchIdx = -1;

    for (size_t i = 0; i < selectorKeys.size(); ++i)
    {
        if (leadToken == juce::String(selectorKeys[i]))
        {
            matchIdx = static_cast<int>(i);
            break;
        }
    }

    if (matchIdx >= 0)
    {
        // Strip leading token and emit remaining arguments
        juce::String remaining = "";
        for (int t = 1; t < tokens.size(); ++t)
        {
            if (!remaining.isEmpty()) remaining += " ";
            remaining += tokens[t];
        }
        emitMessageOnOutlet(matchIdx, remaining.isEmpty() ? "bang" : remaining.toStdString());
    }
    else
    {
        // Emit full original message out rightmost outlet
        int rightmostOutlet = static_cast<int>(selectorKeys.size());
        emitMessageOnOutlet(rightmostOutlet, msgStr.toStdString());
    }
}


// ============================================================================
// LineTildeNode ([line~])
// ============================================================================

LineTildeNode::LineTildeNode(int id, double initialValue)
    : RelativisticNode(id, "line~", "line~"), currentValue(initialValue), targetValue(initialValue)
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
    addOutlet("out~", PortDataType::Audio);
}

void LineTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    samplesRemaining = 0;
    stepPerSample = 0.0;
    segmentQueue.clear();
}

void LineTildeNode::setTarget(double target, double timeMs)
{
    targetValue = target;
    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    if (timeMs <= 0.0)
    {
        currentValue = target;
        samplesRemaining = 0;
        stepPerSample = 0.0;
    }
    else
    {
        samplesRemaining = std::max(1, static_cast<int>(timeMs * 0.001 * sRate));
        stepPerSample = (targetValue - currentValue) / static_cast<double>(samplesRemaining);
    }
}

void LineTildeNode::processNextSegment()
{
    if (segmentQueue.empty()) return;
    auto seg = segmentQueue.front();
    segmentQueue.erase(segmentQueue.begin());
    setTarget(seg.targetValue, seg.durationMs);
}

void LineTildeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ,", "");

    if (tokens.size() == 1)
    {
        // Jump directly to target
        setTarget(tokens[0].getDoubleValue(), 0.0);
        segmentQueue.clear();
    }
    else if (tokens.size() == 2)
    {
        // Standard "<target> <time_ms>"
        setTarget(tokens[0].getDoubleValue(), tokens[1].getDoubleValue());
        segmentQueue.clear();
    }
    else if (tokens.size() >= 3)
    {
        // Multi-segment ramp list: e.g. "0, 1 100 0 200"
        segmentQueue.clear();
        setTarget(tokens[0].getDoubleValue(), 0.0);

        for (int i = 1; i + 1 < tokens.size(); i += 2)
        {
            Segment seg;
            seg.targetValue = tokens[i].getDoubleValue();
            seg.durationMs = tokens[i + 1].getDoubleValue();
            segmentQueue.push_back(seg);
        }
        processNextSegment();
    }
}

void LineTildeNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    auto& outBuf = getAudioOutlet("out~");
    if (outBuf.getNumChannels() < 2 || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(2, numSamples, false, false, true);
    }

    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double currentGamma = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        double effectiveGamma = std::clamp(currentGamma, -100.0, 100.0);

        if (samplesRemaining > 0)
        {
            currentValue += stepPerSample * effectiveGamma;
            --samplesRemaining;

            if (samplesRemaining == 0)
            {
                currentValue = targetValue;
                processNextSegment();
            }
        }

        if (std::abs(currentValue) < 1.0e-15) currentValue = 0.0;

        float val = static_cast<float>(currentValue);
        outL[s] = val;
        outR[s] = val;
    }
}


// ============================================================================
// MetroNode ([metro])
// ============================================================================

MetroNode::MetroNode(int id, double intervalMs, bool autoStart)
    : RelativisticNode(id, "metro", "metro " + std::to_string(static_cast<int>(intervalMs))),
      intervalMsVal(intervalMs), isRunning(autoStart)
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
}

void MetroNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    sampleAccumulator = 0.0;
}

void MetroNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    if (msg == "1" || msg == "start" || msg == "bang")
    {
        start();
    }
    else if (msg == "0" || msg == "stop")
    {
        stop();
    }
    else
    {
        try
        {
            double ms = std::stod(msg.toStdString());
            setIntervalMs(ms);
        }
        catch (...) {}
    }
}

void MetroNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    if (!isRunning) return;

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double periodSamples = std::max(1.0, intervalMsVal * 0.001 * sRate);

    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double currentGamma = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        sampleAccumulator += std::max(0.0, currentGamma);

        if (sampleAccumulator >= periodSamples)
        {
            sampleAccumulator -= periodSamples;
            emitMessageOnMsgOut("bang");
        }
    }
}


// ============================================================================
// DelNode ([del], [delay])
// ============================================================================

DelNode::DelNode(int id, double delayMs)
    : RelativisticNode(id, "del", "del " + std::to_string(static_cast<int>(delayMs))), defaultDelayMs(delayMs)
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
}

void DelNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    isPending = false;
    sampleCountdown = 0.0;
}

void DelNode::trigger(double delayMs)
{
    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    sampleCountdown = std::max(1.0, delayMs * 0.001 * sRate);
    isPending = true;
}

void DelNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    if (msg == "stop")
    {
        stop();
    }
    else if (msg == "bang")
    {
        trigger(defaultDelayMs);
    }
    else
    {
        try
        {
            double ms = std::stod(msg.toStdString());
            trigger(ms);
        }
        catch (...)
        {
            trigger(defaultDelayMs);
        }
    }
}

void DelNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    if (!isPending) return;

    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double currentGamma = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        sampleCountdown -= std::max(0.0, currentGamma);

        if (sampleCountdown <= 0.0)
        {
            isPending = false;
            emitMessageOnMsgOut("bang");
            break;
        }
    }
}


// ============================================================================
// RandomNode ([random])
// ============================================================================

RandomNode::RandomNode(int id, int maxVal)
    : RelativisticNode(id, "random", "random " + std::to_string(maxVal)), maxRange(std::max(1, maxVal))
{
    addInlet("msgIn", PortDataType::Message);
    addOutlet("msgOut", PortDataType::Message);
}

void RandomNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void RandomNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void RandomNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    if (msg == "bang")
    {
        std::uniform_int_distribution<int> dist(0, maxRange - 1);
        int val = dist(rng);
        emitMessageOnMsgOut(std::to_string(val));
    }
    else
    {
        try
        {
            int newMax = std::stoi(msg.toStdString());
            setMax(newMax);
        }
        catch (...) {}
    }
}


// ============================================================================
// CounterNode ([counter])
// ============================================================================

CounterNode::CounterNode(int id, int minVal, int maxVal, int step)
    : RelativisticNode(id, "counter", "counter"), minRange(minVal), maxRange(maxVal), stepSize(step), currentVal(minVal)
{
    addInlet("msgIn", PortDataType::Message);
    addOutlet("msgOut", PortDataType::Message);
}

void CounterNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    currentVal = minRange;
}

void CounterNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void CounterNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    if (msg == "bang")
    {
        emitMessageOnMsgOut(std::to_string(currentVal));

        currentVal += direction * stepSize;
        if (currentVal > maxRange)
        {
            currentVal = minRange;
        }
        else if (currentVal < minRange)
        {
            currentVal = maxRange;
        }
    }
    else if (msg == "reset" || msg == "0")
    {
        reset();
    }
    else if (msg == "up")
    {
        direction = 1;
    }
    else if (msg == "down")
    {
        direction = -1;
    }
    else if (msg.startsWith("set "))
    {
        currentVal = msg.substring(4).getIntValue();
    }
}

} // namespace TimeDilationDAW
