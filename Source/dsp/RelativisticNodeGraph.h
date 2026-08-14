#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include "TimePolyFrame.h"

namespace TimeDilationDAW
{

enum class PortDataType
{
    Audio,   // Cyber Cyan (#06b6d4)
    Time,    // Royal Violet (#8b5cf6)
    Message  // Gold Accent (#eab308)
};

enum class PortDirection
{
    Inlet,
    Outlet
};

struct Port
{
    int id = 0;
    std::string name;
    PortDataType dataType = PortDataType::Audio;
    PortDirection direction = PortDirection::Inlet;
    int nodeOwnerId = 0;
    int portIndex = 0;
};

struct PatchConnection
{
    int connectionId = 0;
    int sourceNodeId = 0;
    int sourcePortIndex = 0;
    int destNodeId = 0;
    int destPortIndex = 0;
    PortDataType dataType = PortDataType::Audio;
    bool isFeedbackCycle = false;
    double lastMessageTriggerTime = -100.0;

    // Per-connection Feedback Protection (DC Blocker & Soft Clipper)
    bool enableSoftClip = true;
    bool enableDcBlock = true;
    float dcX1[2] = { 0.0f, 0.0f };
    float dcY1[2] = { 0.0f, 0.0f };
};

class RelativisticAudioHistoryBuffer
{
public:
    void prepare(double sampleRate, double maxHistorySeconds = 10.0)
    {
        int totalSamples = static_cast<int>(sampleRate * maxHistorySeconds);
        buffer.setSize(2, std::max(44100, totalSamples));
        buffer.clear();
        writeIndex = 0;
        bufferLength = buffer.getNumSamples();
    }

    void writeBlock(const juce::AudioBuffer<float>& inBuf, int numSamples)
    {
        if (bufferLength == 0) return;
        int channels = std::min(2, inBuf.getNumChannels());
        for (int ch = 0; ch < channels; ++ch)
        {
            const float* src = inBuf.getReadPointer(ch);
            float* dest = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                dest[(writeIndex + i) % bufferLength] = src[i];
            }
        }
        writeIndex = (writeIndex + numSamples) % bufferLength;
    }

    float readPastSample(int channel, double pastOffsetSamples) const
    {
        if (bufferLength == 0) return 0.0f;
        double readPos = static_cast<double>(writeIndex) - pastOffsetSamples;
        while (readPos < 0.0) readPos += bufferLength;
        while (readPos >= bufferLength) readPos -= bufferLength;

        int i0 = static_cast<int>(readPos);
        int i1 = (i0 + 1) % bufferLength;
        float frac = static_cast<float>(readPos - i0);

        const float* data = buffer.getReadPointer(channel % buffer.getNumChannels());
        return (1.0f - frac) * data[i0] + frac * data[i1];
    }

private:
    juce::AudioBuffer<float> buffer;
    int writeIndex = 0;
    int bufferLength = 0;
};

struct RelativisticControlEvent
{
    double tauTimestamp = 0.0;
    std::string message;
    double floatVal = 0.0;
};

class RelativisticControlPipe
{
public:
    void addEvent(double tau, const std::string& msg, double val = 0.0)
    {
        events.push_back({ tau, msg, val });
        if (events.size() > 1000) events.erase(events.begin());
    }

    double getValueAtTau(double tau) const
    {
        if (events.empty()) return 0.0;
        double lastVal = events.front().floatVal;
        for (const auto& ev : events)
        {
            if (ev.tauTimestamp > tau) break;
            lastVal = ev.floatVal;
        }
        return lastVal;
    }

    const std::vector<RelativisticControlEvent>& getEvents() const { return events; }

private:
    std::vector<RelativisticControlEvent> events;
};

class RelativisticNode
{
public:
    RelativisticNode(int id, const std::string& symbol, const std::string& label = "");
    virtual ~RelativisticNode() = default;

    int getId() const { return nodeId; }
    std::string getSymbol() const { return symbolType; }
    std::string getLabel() const { return nodeLabel; }
    void setLabel(const std::string& l) { nodeLabel = l; }

    void addInlet(const std::string& name, PortDataType type);
    void addOutlet(const std::string& name, PortDataType type);

    const std::vector<Port>& getInlets() const { return inlets; }
    const std::vector<Port>& getOutlets() const { return outlets; }

    bool isInletConnected(int inletIdx) const
    {
        if (inletIdx >= 0 && inletIdx < static_cast<int>(inletConnectedFlags.size()))
            return inletConnectedFlags[static_cast<size_t>(inletIdx)];
        return false;
    }
    void setInletConnected(int inletIdx, bool connected)
    {
        if (inletIdx >= 0 && inletIdx < static_cast<int>(inletConnectedFlags.size()))
            inletConnectedFlags[static_cast<size_t>(inletIdx)] = connected;
    }

    virtual void prepare(double sampleRate, int samplesPerBlock);
    virtual void process(int numSamples) = 0;
    virtual void receiveMessage(const std::string& message);

    RelativisticAudioHistoryBuffer audioHistory;
    RelativisticControlPipe controlPipe;

    std::function<void(const std::string&)> onMessageEmitted;
    std::function<void(int, const std::string&)> onOutletMessageEmitted;

    void emitMessageOnMsgOut(const std::string& msg)
    {
        emitMessageOnOutlet(0, msg);
    }

    void emitMessageOnOutlet(int outletIdx, const std::string& msg)
    {
        if (onOutletMessageEmitted)
            onOutletMessageEmitted(outletIdx, msg);
        if (outletIdx == 0 && onMessageEmitted)
            onMessageEmitted(msg);
    }

    // Buffer access
    juce::AudioBuffer<float>& getOutletBuffer(int index);
    const juce::AudioBuffer<float>& getInletBuffer(int index) const;

    // Type-Safe Named Port Lookup System
    int getInletIndex(const std::string& name) const;
    int getOutletIndex(const std::string& name) const;

    juce::AudioBuffer<float>& getAudioOutlet(const std::string& name);
    const juce::AudioBuffer<float>& getAudioInlet(const std::string& name) const;

    TimePolyFrame& getTimeOutlet(const std::string& name);
    const TimePolyFrame& getTimeInlet(const std::string& name) const;
    
    // Time stream access
    TimePolyFrame& getOutletTimeFrame(int index);
    const TimePolyFrame& getInletTimeFrame(int index) const;

    void setInletBufferData(int portIndex, const juce::AudioBuffer<float>& data);
    void setInletTimeFrameData(int portIndex, const TimePolyFrame& frame);

    // Position & Dimensions for GUI
    float xPos = 100.0f;
    float yPos = 100.0f;
    float width = 130.0f;
    float height = 45.0f;

    // Per-Node Output Gain Staging Volume (Linear gain with Logarithmic dBFS helpers)
    float outputVolume = 1.0f;
    float getOutputVolume() const { return outputVolume; }
    void setOutputVolume(float vol) { outputVolume = std::clamp(vol, 0.0f, 4.0f); }

    float getVolumeDb() const
    {
        return (outputVolume <= 0.00001f) ? -100.0f : juce::Decibels::gainToDecibels(outputVolume, -100.0f);
    }
    void setVolumeDb(float db)
    {
        if (db <= -99.0f) setOutputVolume(0.0f);
        else setOutputVolume(juce::Decibels::decibelsToGain(db, -100.0f));
    }

    // Dual Real-Time Scope Display Buffers (Audio Output & Proper Time Telemetry)
    std::vector<float> audioScopeBuffer;
    size_t audioScopeWriteIdx = 0;

    std::vector<float> timeScopeBuffer;
    size_t timeScopeWriteIdx = 0;

    std::vector<float> timeTauScopeBuffer;
    size_t timeTauScopeWriteIdx = 0;

    // Legacy fallback accessor
    const std::vector<float>& getScopeHistoryBuffer() const { return (displayType == ScopeDisplayType::AudioWaveform) ? audioScopeBuffer : timeScopeBuffer; }
    size_t getScopeHistoryWriteIdx() const { return (displayType == ScopeDisplayType::AudioWaveform) ? audioScopeWriteIdx : timeScopeWriteIdx; }

    // Real-Time Audio Telemetry (RMS & Peak) for Users & AI Terminal
    std::atomic<float> rmsLevel{ 0.0f };
    std::atomic<float> peakLevel{ 0.0f };

    float getRmsLevel() const { return rmsLevel.load(); }
    float getPeakLevel() const { return peakLevel.load(); }

    void updateAudioTelemetry(const juce::AudioBuffer<float>& buf, int numSamples)
    {
        if (numSamples <= 0 || buf.getNumChannels() == 0) return;
        float rms = buf.getRMSLevel(0, 0, numSamples);
        float mag = buf.getMagnitude(0, numSamples);
        if (buf.getNumChannels() > 1)
        {
            rms = std::max(rms, buf.getRMSLevel(1, 0, numSamples));
            mag = std::max(mag, buf.getMagnitude(1, numSamples));
        }
        float prevRms = rmsLevel.load();
        float prevPeak = peakLevel.load();
        rmsLevel.store(std::max(rms, prevRms * 0.82f));
        peakLevel.store(std::max(mag, prevPeak * 0.82f));
    }

    void pushAudioScopeSample(float sample)
    {
        if (audioScopeBuffer.size() < 256) audioScopeBuffer.assign(256, 0.0f);
        audioScopeBuffer[audioScopeWriteIdx] = sample;
        audioScopeWriteIdx = (audioScopeWriteIdx + 1) % audioScopeBuffer.size();
    }

    void pushTimeScopeSample(float sample)
    {
        if (timeScopeBuffer.size() < 256) timeScopeBuffer.assign(256, 0.0f);
        timeScopeBuffer[timeScopeWriteIdx] = sample;
        timeScopeWriteIdx = (timeScopeWriteIdx + 1) % timeScopeBuffer.size();
    }

    void pushTimeTauScopeSample(float sample)
    {
        if (timeTauScopeBuffer.size() < 256) timeTauScopeBuffer.assign(256, 0.0f);
        timeTauScopeBuffer[timeTauScopeWriteIdx] = sample;
        timeTauScopeWriteIdx = (timeTauScopeWriteIdx + 1) % timeTauScopeBuffer.size();
    }

    void pushScopeSample(float sample)
    {
        pushTimeScopeSample(sample);
        pushAudioScopeSample(sample);
    }

    // Toggleable Realtime Scope & Visualizer Display
    bool showRealtimeDisplay = true;
    enum class ScopeRenderMode { Waveform2D, ScopeXY, Scope3D };
    enum class ScopeDisplayType { AudioWaveform, TimeFrame };
    enum class TimeScopeVariable { SpeedGamma, OffsetTau, CouplingC, MultiTime };

    enum class TimeCouplingMode { Both, SpeedOnly, OffsetOnly, Bypassed };
    TimeCouplingMode timeCouplingMode = TimeCouplingMode::Both;
    double offsetCouplingFactor = 1.0; // 0.0 = Decoupled, 1.0 = Full Proper-Time Displacement

    TimeCouplingMode getTimeCouplingMode() const { return timeCouplingMode; }
    void setTimeCouplingMode(TimeCouplingMode mode) { timeCouplingMode = mode; }
    double getOffsetCouplingFactor() const { return offsetCouplingFactor; }
    void setOffsetCouplingFactor(double c) { offsetCouplingFactor = std::clamp(c, 0.0, 1.0); }

    ScopeRenderMode scopeMode = ScopeRenderMode::Waveform2D;
    ScopeDisplayType displayType = ScopeDisplayType::AudioWaveform;
    TimeScopeVariable timeVarMode = TimeScopeVariable::SpeedGamma;

    void toggleRealtimeDisplay() { showRealtimeDisplay = !showRealtimeDisplay; }
    void cycleScopeMode()
    {
        if (scopeMode == ScopeRenderMode::Waveform2D) scopeMode = ScopeRenderMode::ScopeXY;
        else if (scopeMode == ScopeRenderMode::ScopeXY) scopeMode = ScopeRenderMode::Scope3D;
        else scopeMode = ScopeRenderMode::Waveform2D;
    }

    void cycleScopeDisplayMode()
    {
        bool hasAudioPorts = false;
        for (const auto& out : outlets) {
            if (out.dataType == PortDataType::Audio) { hasAudioPorts = true; break; }
        }
        if (!hasAudioPorts) {
            for (const auto& in : inlets) {
                if (in.dataType == PortDataType::Audio) { hasAudioPorts = true; break; }
            }
        }

        if (hasAudioPorts)
        {
            if (displayType == ScopeDisplayType::AudioWaveform)
            {
                displayType = ScopeDisplayType::TimeFrame;
                timeVarMode = TimeScopeVariable::SpeedGamma;
            }
            else if (timeVarMode == TimeScopeVariable::SpeedGamma)
            {
                timeVarMode = TimeScopeVariable::OffsetTau;
            }
            else if (timeVarMode == TimeScopeVariable::OffsetTau)
            {
                timeVarMode = TimeScopeVariable::CouplingC;
            }
            else
            {
                displayType = ScopeDisplayType::AudioWaveform;
                timeVarMode = TimeScopeVariable::SpeedGamma;
            }
        }
        else
        {
            displayType = ScopeDisplayType::TimeFrame;
            if (timeVarMode == TimeScopeVariable::SpeedGamma) timeVarMode = TimeScopeVariable::OffsetTau;
            else if (timeVarMode == TimeScopeVariable::OffsetTau) timeVarMode = TimeScopeVariable::CouplingC;
            else if (timeVarMode == TimeScopeVariable::CouplingC) timeVarMode = TimeScopeVariable::MultiTime;
            else timeVarMode = TimeScopeVariable::SpeedGamma;
        }
    }

    void cycleTimeVarMode()
    {
        cycleScopeDisplayMode();
    }

protected:
    int nodeId;
    std::string symbolType;
    std::string nodeLabel;

    std::vector<Port> inlets;
    std::vector<Port> outlets;

    std::vector<juce::AudioBuffer<float>> inletBuffers;
    std::vector<juce::AudioBuffer<float>> outletBuffers;

    std::vector<TimePolyFrame> inletTimeFrames;
    std::vector<TimePolyFrame> outletTimeFrames;
    std::vector<bool> inletConnectedFlags;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
};

class RelativisticPreCausalBuffer
{
public:
    void prepare(double sampleRate, double maxLookAheadSeconds = 10.0)
    {
        int totalSamples = static_cast<int>(sampleRate * maxLookAheadSeconds);
        buffer.setSize(2, std::max(44100, totalSamples));
        buffer.clear();
        lookAheadSamples = buffer.getNumSamples();
        writePos = 0;
    }

    void writeBlock(const juce::AudioBuffer<float>& inBuf, int numSamples)
    {
        if (buffer.getNumSamples() == 0) return;
        int channels = std::min(2, inBuf.getNumChannels());
        int len = buffer.getNumSamples();
        for (int ch = 0; ch < channels; ++ch)
        {
            const float* src = inBuf.getReadPointer(ch);
            float* dest = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                dest[(writePos + i) % len] = src[i];
            }
        }
        writePos = (writePos + numSamples) % len;
    }

    int getWritePos() const { return writePos; }

    float readFutureSample(int channel, double futureOffsetSamples) const
    {
        return readFutureSampleHermiteAtPos(channel, static_cast<double>(writePos) - futureOffsetSamples);
    }

    float readFutureSampleHermiteAtPos(int channel, double absoluteReadPos) const
    {
        if (buffer.getNumSamples() == 0) return 0.0f;
        int len = buffer.getNumSamples();
        double readPos = absoluteReadPos;
        while (readPos < 0.0) readPos += len;
        while (readPos >= len) readPos -= len;

        int i0 = static_cast<int>(std::floor(readPos)) % len;
        if (i0 < 0) i0 += len;
        double f = readPos - std::floor(readPos);

        int im1 = (i0 - 1 + len) % len;
        int i1  = (i0 + 1) % len;
        int i2  = (i0 + 2) % len;

        const float* data = buffer.getReadPointer(channel % buffer.getNumChannels());
        double ym1 = data[im1];
        double y0  = data[i0];
        double y1  = data[i1];
        double y2  = data[i2];

        double c0 = y0;
        double c1 = 0.5 * (y1 - ym1);
        double c2 = ym1 - 2.5 * y0 + 2.0 * y1 - 0.5 * y2;
        double c3 = 0.5 * (y2 - ym1) + 1.5 * (y0 - y1);

        return static_cast<float>(((c3 * f + c2) * f + c1) * f + c0);
    }

    int getLookAheadSamples() const { return lookAheadSamples; }

private:
    juce::AudioBuffer<float> buffer;
    int lookAheadSamples = 441000;
    int writePos = 0;
};

class AdaptiveLatencyEngine
{
public:
    void prepare(double sr)
    {
        sampleRate = sr;
        currentLatencySamples = 512.0; // Low startup latency (~5.3ms)
        targetLatencySamples = 512.0;
        rampProgress = 1.0;
    }

    void updateDemand(double maxFutureOffsetSec)
    {
        double requiredSec = std::max(0.0053, maxFutureOffsetSec);
        double newTarget = requiredSec * sampleRate;
        if (std::abs(newTarget - targetLatencySamples) > 64.0)
        {
            startLatencySamples = currentLatencySamples;
            targetLatencySamples = newTarget;
            rampProgress = 0.0;
        }
    }

    double advanceSampleSmooth()
    {
        if (rampProgress < 1.0)
        {
            double step = 1.0 / (sampleRate * 0.15); // 150ms ultra-smooth C2 Hermite S-Curve
            rampProgress = std::min(1.0, rampProgress + step);
            double u = rampProgress;
            double sCurve = 3.0 * u * u - 2.0 * u * u * u;
            currentLatencySamples = startLatencySamples + (targetLatencySamples - startLatencySamples) * sCurve;
        }
        else
        {
            currentLatencySamples = targetLatencySamples;
        }
        return currentLatencySamples;
    }

    double advanceSmoothCurvature(int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            advanceSampleSmooth();
        }
        return currentLatencySamples;
    }

    double getCurrentLatencyMs() const { return (currentLatencySamples / sampleRate) * 1000.0; }
    double getCurrentLatencySamples() const { return currentLatencySamples; }
    double getTargetLatencySamples() const { return targetLatencySamples; }
    double getRampProgress() const { return rampProgress; }
    bool isRamping() const { return rampProgress < 1.0; }

private:
    double sampleRate = 96000.0;
    double currentLatencySamples = 512.0;
    double startLatencySamples = 512.0;
    double targetLatencySamples = 512.0;
    double rampProgress = 1.0;
};

class RelativisticNodeGraph
{
public:
    RelativisticNodeGraph();
    ~RelativisticNodeGraph() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& masterOutBuffer, int numSamples);

    int addNode(std::shared_ptr<RelativisticNode> node);
    bool removeNode(int nodeId);
    void clearGraph();

    bool addConnection(int srcNodeId, int srcPortIdx, int destNodeId, int destPortIdx);
    bool removeConnection(int connectionId);

    std::shared_ptr<RelativisticNode> getNode(int nodeId);
    const std::vector<std::shared_ptr<RelativisticNode>>& getNodes() const { return nodes; }
    const std::vector<PatchConnection>& getConnections() const { return connections; }

    void updateTopologicalSort();

    int getLookAheadLatencySamples() const { return static_cast<int>(latencyEngine.getCurrentLatencySamples()); }
    double getAdaptiveLatencyMs() const { return latencyEngine.getCurrentLatencyMs(); }

    void setAudioPlayHead(juce::AudioPlayHead* playHead) { audioPlayHead = playHead; }
    juce::AudioPlayHead* getAudioPlayHead() const { return audioPlayHead; }

    void setManualLatencyDemand(double demandSec)
    {
        manualDemandSec = demandSec;
        latencyEngine.updateDemand(demandSec);
    }

    void resetManualLatencyDemand()
    {
        manualDemandSec = -1.0;
    }

    double getManualLatencyDemand() const { return manualDemandSec; }
    RelativisticPreCausalBuffer& getPreCausalBuffer() { return preCausalBuffer; }

    // Feedback Loop Protection & DC Blocking (User-toggleable)
    void setFeedbackProtectionEnabled(bool enabled) { feedbackSoftClipEnabled = enabled; feedbackDcBlockEnabled = enabled; }
    bool isFeedbackProtectionEnabled() const { return feedbackSoftClipEnabled || feedbackDcBlockEnabled; }

    void setFeedbackSoftClipEnabled(bool enabled) { feedbackSoftClipEnabled = enabled; }
    bool isFeedbackSoftClipEnabled() const { return feedbackSoftClipEnabled; }

    void setFeedbackDcBlockEnabled(bool enabled) { feedbackDcBlockEnabled = enabled; }
    bool isFeedbackDcBlockEnabled() const { return feedbackDcBlockEnabled; }

    void setConnectionFeedbackProtection(int connectionId, bool softClip, bool dcBlock);

    // Composite node sub-graph helper
    std::string serializeToJSON() const;
    bool deserializeFromJSON(const std::string& jsonStr);

private:
    double manualDemandSec = -1.0;
    juce::AudioPlayHead* audioPlayHead = nullptr;

private:
    std::vector<std::shared_ptr<RelativisticNode>> nodes;
    std::unordered_map<int, std::shared_ptr<RelativisticNode>> nodeMap;
    std::vector<PatchConnection> connections;
    int nextConnectionId = 1;

    std::vector<std::shared_ptr<RelativisticNode>> sortedNodes;

    // Tarjan SCC Cycle Resolution Buffers
    std::unordered_map<int, std::vector<juce::AudioBuffer<float>>> previousBlockBuffers;
    std::unordered_map<int, std::vector<TimePolyFrame>> previousBlockTimeFrames;

    // Feedback Loop Protection & DC Filter Settings
    bool feedbackSoftClipEnabled = true;
    bool feedbackDcBlockEnabled = true;
    float dcR = 0.99967f;
    juce::AudioBuffer<float> feedbackScratchBuffer;

    RelativisticPreCausalBuffer preCausalBuffer;
    AdaptiveLatencyEngine latencyEngine;
    double sampleRate = 96000.0; // Professional Studio 96kHz default!
    int samplesPerBlock = 512;

    void runTarjanSCC();
};

} // namespace TimeDilationDAW
