#include "PdDelayNodes.h"
#include "../utils/ConsoleLogger.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace TimeDilationDAW
{

// ============================================================================
// DelayLineManager Implementation
// ============================================================================

void DelayLineManager::allocate(const std::string& name, double maxDurationMs, double sampleRate)
{
    std::lock_guard<std::mutex> lock(mutex);
    size_t requiredSamples = static_cast<size_t>(std::max(1.0, (maxDurationMs * 0.001 * sampleRate) + 64.0));
    
    auto& line = delayLines[name];
    if (line.size < requiredSamples)
    {
        line.bufferL.assign(requiredSamples, 0.0f);
        line.bufferR.assign(requiredSamples, 0.0f);
        line.size = requiredSamples;
        line.writeIdx = 0;
        line.sampleRate = sampleRate;
    }
}

bool DelayLineManager::hasLine(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return delayLines.find(name) != delayLines.end();
}

void DelayLineManager::clearAll()
{
    std::lock_guard<std::mutex> lock(mutex);
    delayLines.clear();
}

void DelayLineManager::writeSample(const std::string& name, float sampleL, float sampleR)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = delayLines.find(name);
    if (it == delayLines.end() || it->second.size == 0) return;

    auto& line = it->second;
    line.bufferL[line.writeIdx] = sampleL;
    line.bufferR[line.writeIdx] = sampleR;
    line.writeIdx = (line.writeIdx + 1) % line.size;
}

void DelayLineManager::writeBlock(const std::string& name, const juce::AudioBuffer<float>& buffer, int numSamples)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = delayLines.find(name);
    if (it == delayLines.end() || it->second.size == 0) return;

    auto& line = it->second;
    const float* inL = buffer.getReadPointer(0);
    const float* inR = (buffer.getNumChannels() > 1) ? buffer.getReadPointer(1) : inL;

    for (int s = 0; s < numSamples; ++s)
    {
        line.bufferL[line.writeIdx] = inL[s];
        line.bufferR[line.writeIdx] = inR[s];
        line.writeIdx = (line.writeIdx + 1) % line.size;
    }
}

static inline float hermiteInterpolate(float ym1, float y0, float y1, float y2, float frac)
{
    float c0 = y0;
    float c1 = 0.5f * (y1 - ym1);
    float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void DelayLineManager::readInterpolated(const std::string& name, double delaySamples, float& outL, float& outR) const
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = delayLines.find(name);
    if (it == delayLines.end() || it->second.size == 0)
    {
        outL = 0.0f;
        outR = 0.0f;
        return;
    }

    const auto& line = it->second;
    double maxDelay = static_cast<double>(line.size - 4);
    double clampedDelay = std::clamp(delaySamples, 0.0, maxDelay);

    double readPos = static_cast<double>(line.writeIdx) - clampedDelay;
    while (readPos < 0.0) readPos += static_cast<double>(line.size);

    int idx0 = static_cast<int>(std::floor(readPos)) % static_cast<int>(line.size);
    float frac = static_cast<float>(readPos - std::floor(readPos));

    int idxM1 = (idx0 - 1 + static_cast<int>(line.size)) % static_cast<int>(line.size);
    int idx1  = (idx0 + 1) % static_cast<int>(line.size);
    int idx2  = (idx0 + 2) % static_cast<int>(line.size);

    outL = hermiteInterpolate(line.bufferL[static_cast<size_t>(idxM1)],
                              line.bufferL[static_cast<size_t>(idx0)],
                              line.bufferL[static_cast<size_t>(idx1)],
                              line.bufferL[static_cast<size_t>(idx2)],
                              frac);

    outR = hermiteInterpolate(line.bufferR[static_cast<size_t>(idxM1)],
                              line.bufferR[static_cast<size_t>(idx0)],
                              line.bufferR[static_cast<size_t>(idx1)],
                              line.bufferR[static_cast<size_t>(idx2)],
                              frac);
}

void DelayLineManager::readBlock(const std::string& name, double delaySamples, juce::AudioBuffer<float>& outBuffer, int numSamples) const
{
    float* outL = outBuffer.getWritePointer(0);
    float* outR = (outBuffer.getNumChannels() > 1) ? outBuffer.getWritePointer(1) : outL;

    for (int s = 0; s < numSamples; ++s)
    {
        float l = 0.0f, r = 0.0f;
        readInterpolated(name, delaySamples, l, r);
        outL[s] = l;
        outR[s] = r;
    }
}


// ============================================================================
// DelwriteTildeNode ([delwrite~ <name> <max_ms>])
// ============================================================================

DelwriteTildeNode::DelwriteTildeNode(int id, const std::string& name, double maxDurationMs)
    : RelativisticNode(id, "delwrite~", "delwrite~ " + name + " " + std::to_string(static_cast<int>(maxDurationMs))),
      delayName(name), maxDurationMs(maxDurationMs)
{
    addInlet("in~", PortDataType::Audio);     // Inlet 1: Audio Signal to Write
    addInlet("timeIn", PortDataType::Time);   // Inlet 2: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
    addOutlet("out~", PortDataType::Audio);   // Outlet 2: Audio Passthrough Output
}

void DelwriteTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    DelayLineManager::getInstance().allocate(delayName, maxDurationMs, sampleRate);
}

void DelwriteTildeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() >= 2 && tokens[0] == "set")
    {
        delayName = tokens[1].toStdString();
        DelayLineManager::getInstance().allocate(delayName, maxDurationMs, currentSampleRate);
    }
    else if (tokens.size() >= 2 && tokens[0] == "size")
    {
        maxDurationMs = tokens[1].getDoubleValue();
        DelayLineManager::getInstance().allocate(delayName, maxDurationMs, currentSampleRate);
    }
}

void DelwriteTildeNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    const auto& inBuf = getInletBuffer(1);
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    // Write incoming block to named shared delay line
    DelayLineManager::getInstance().writeBlock(delayName, inBuf, numSamples);

    // Pass through audio to outlet
    auto& outBuf = getAudioOutlet("out~");
    if (outBuf.getNumChannels() < 2 || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(2, numSamples, false, false, true);
    }
    outBuf.makeCopyOf(inBuf);
}


// ============================================================================
// DelreadTildeNode ([delread~ <name> <delay_ms>])
// ============================================================================

DelreadTildeNode::DelreadTildeNode(int id, const std::string& name, double delayMs)
    : RelativisticNode(id, "delread~", "delread~ " + name + " " + std::to_string(static_cast<int>(delayMs))),
      delayName(name), delayMs(delayMs)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
    addOutlet("out~", PortDataType::Audio);   // Outlet 2: Delayed Audio Output
}

void DelreadTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void DelreadTildeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { setDelayMs(tokens[0].getDoubleValue()); } catch (...) {}
    }
    else if (tokens.size() >= 2 && tokens[0] == "set")
    {
        delayName = tokens[1].toStdString();
    }
}

void DelreadTildeNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    auto& outBuf = getAudioOutlet("out~");
    if (outBuf.getNumChannels() < 2 || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(2, numSamples, false, false, true);
    }

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double delaySamples = (delayMs * 0.001 * sRate);

    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    for (int s = 0; s < numSamples; ++s)
    {
        float l = 0.0f, r = 0.0f;
        DelayLineManager::getInstance().readInterpolated(delayName, delaySamples, l, r);
        outL[s] = l;
        outR[s] = r;
    }
}


// ============================================================================
// VdTildeNode ([vd~ <name>], [time.vd~ <name>])
// ============================================================================

VdTildeNode::VdTildeNode(int id, const std::string& name, double defaultDelayMs)
    : RelativisticNode(id, "vd~", "vd~ " + name), delayName(name), baseDelayMs(defaultDelayMs), smoothedDelayMs(defaultDelayMs)
{
    addInlet("delayMod~", PortDataType::Audio); // Inlet 1: Delay Time in ms (Audio Signal)
    addInlet("timeIn", PortDataType::Time);     // Inlet 2: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time);   // Outlet 1: Time Frame Output
    addOutlet("out~", PortDataType::Audio);     // Outlet 2: Variable Doppler Audio Output
}

void VdTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    smoothedDelayMs = baseDelayMs;
}

void VdTildeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { setBaseDelayMs(tokens[0].getDoubleValue()); } catch (...) {}
    }
    else if (tokens.size() >= 2 && tokens[0] == "set")
    {
        delayName = tokens[1].toStdString();
    }
}

void VdTildeNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    const auto& modBuf = getInletBuffer(1);
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    auto& outBuf = getAudioOutlet("out~");
    if (outBuf.getNumChannels() < 2 || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(2, numSamples, false, false, true);
    }

    const float* modL = modBuf.getReadPointer(0);
    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double currentGamma = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        double effectiveGamma = std::clamp(currentGamma, 0.001, 100.0);

        // Modulate delay by audio inlet if provided, else use baseDelayMs
        double targetDelayMs = (modBuf.getMagnitude(0, numSamples) > 0.0f) ? static_cast<double>(modL[s]) : baseDelayMs;
        if (targetDelayMs < 0.0) targetDelayMs = 0.0;

        // Smooth delay time transitions to prevent clicks
        smoothedDelayMs += 0.005 * (targetDelayMs - smoothedDelayMs);

        // Relativistic proper-time delay calculation:
        // Delay length in samples dynamically scaled by velocity factor
        double delaySamples = (smoothedDelayMs * 0.001 * sRate) / effectiveGamma;

        float l = 0.0f, r = 0.0f;
        DelayLineManager::getInstance().readInterpolated(delayName, delaySamples, l, r);
        outL[s] = l;
        outR[s] = r;
    }
}


// ============================================================================
// PipeNode ([pipe], [time.pipe])
// ============================================================================

PipeNode::PipeNode(int id, double defaultDelayMs)
    : RelativisticNode(id, "pipe", "pipe " + std::to_string(static_cast<int>(defaultDelayMs))),
      defaultDelayMs(defaultDelayMs)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
}

void PipeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    clear();
}

void PipeNode::clear()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.clear();
}

void PipeNode::flush()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    for (const auto& ev : eventQueue)
    {
        emitMessageOnMsgOut(ev.payload);
    }
    eventQueue.clear();
}

void PipeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    if (msgStr == "flush")
    {
        flush();
        return;
    }
    if (msgStr == "clear")
    {
        clear();
        return;
    }

    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() >= 2 && tokens[0] == "delay")
    {
        defaultDelayMs = std::max(0.0, tokens[1].getDoubleValue());
        return;
    }

    double delayMs = defaultDelayMs;
    std::string payload = message;

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double reqSamples = std::max(1.0, delayMs * 0.001 * sRate);

    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.push_back({ payload, reqSamples });
}

void PipeNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));
    double gammaDecaySum = 0.0;

    for (int s = 0; s < numSamples; ++s)
    {
        double g = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        gammaDecaySum += std::max(0.0, g);
    }

    std::lock_guard<std::mutex> lock(queueMutex);
    for (auto it = eventQueue.begin(); it != eventQueue.end(); )
    {
        it->remainingSamples -= gammaDecaySum;
        if (it->remainingSamples <= 0.0)
        {
            emitMessageOnMsgOut(it->payload);
            it = eventQueue.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


// ============================================================================
// TimerNode ([timer], [time.timer])
// ============================================================================

TimerNode::TimerNode(int id)
    : RelativisticNode(id, "timer", "timer")
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
}

void TimerNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    resetTimer();
}

void TimerNode::resetTimer()
{
    startProperTimeSec = accumulatedProperTimeSec;
    startCoordinateTimeSec = accumulatedCoordinateTimeSec;
    isRunning = true;
}

double TimerNode::measureElapsedProperTimeMs()
{
    return (accumulatedProperTimeSec - startProperTimeSec) * 1000.0;
}

double TimerNode::measureElapsedCoordinateTimeMs()
{
    return (accumulatedCoordinateTimeSec - startCoordinateTimeSec) * 1000.0;
}

void TimerNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    if (msg == "reset" || msg == "start" || msg == "0")
    {
        resetTimer();
    }
    else if (msg == "bang" || msg == "measure" || msg == "1")
    {
        double elapsedProperMs = measureElapsedProperTimeMs();
        emitMessageOnMsgOut(std::to_string(elapsedProperMs));
    }
}

void TimerNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double dt = 1.0 / sRate;

    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double g = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        accumulatedProperTimeSec += g * dt;
        accumulatedCoordinateTimeSec += dt;
    }
}


// ============================================================================
// SnapshotTildeNode ([snapshot~], [time.snapshot~])
// ============================================================================

SnapshotTildeNode::SnapshotTildeNode(int id)
    : RelativisticNode(id, "snapshot~", "snapshot~")
{
    addInlet("in~", PortDataType::Audio);     // Inlet 1: Audio Signal to sample
    addInlet("timeIn", PortDataType::Time);   // Inlet 2: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
}

void SnapshotTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    lastSampleVal = 0.0f;
}

void SnapshotTildeNode::triggerSample()
{
    emitMessageOnMsgOut(std::to_string(lastSampleVal));
}

void SnapshotTildeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    if (msg == "bang" || msg == "sample" || msg == "read")
    {
        triggerSample();
    }
    else if (msg == "gamma")
    {
        emitMessageOnMsgOut(std::to_string(lastGamma));
    }
    else if (msg == "tau")
    {
        emitMessageOnMsgOut(std::to_string(lastTau));
    }
}

void SnapshotTildeNode::process(int numSamples)
{
    const auto& inBuf = getInletBuffer(1);
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    if (inBuf.getNumSamples() > 0)
    {
        lastSampleVal = inBuf.getSample(0, 0);
    }
    lastGamma = timeFrame.masterGamma;
    lastTau = timeFrame.masterTau;
}


// ============================================================================
// TimeQuantizeNode ([time.quantize])
// ============================================================================

TimeQuantizeNode::TimeQuantizeNode(int id, double divisionMs)
    : RelativisticNode(id, "time.quantize", "time.quantize " + std::to_string(static_cast<int>(divisionMs))),
      gridDivisionMs(divisionMs)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
}

void TimeQuantizeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    currentProperTimeAcc = 0.0;
    std::lock_guard<std::mutex> lock(queueMutex);
    pendingQueue.clear();
}

void TimeQuantizeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    if (msg.startsWith("div") || msg.startsWith("grid"))
    {
        juce::StringArray tokens;
        tokens.addTokens(msg, " ", "");
        if (tokens.size() >= 2)
        {
            setDivisionMs(tokens[1].getDoubleValue());
        }
        return;
    }

    std::lock_guard<std::mutex> lock(queueMutex);
    pendingQueue.push_back({ message });
}

void TimeQuantizeNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double dt = 1.0 / sRate;
    double gridDivisionSec = gridDivisionMs * 0.001;

    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double g = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        currentProperTimeAcc += g * dt;

        if (currentProperTimeAcc >= gridDivisionSec)
        {
            currentProperTimeAcc -= gridDivisionSec;

            // Emit all pending quantized messages
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!pendingQueue.empty())
            {
                for (const auto& item : pendingQueue)
                {
                    emitMessageOnMsgOut(item.payload);
                }
                pendingQueue.clear();
            }
        }
    }
}

} // namespace TimeDilationDAW
