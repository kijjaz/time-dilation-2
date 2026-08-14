#pragma once

#include "RelativisticNodeGraph.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <deque>
#include <string>

namespace TimeDilationDAW
{

// ============================================================================
// DelayLineManager (Shared Named Circular Audio Delay Buffers)
// ============================================================================

class DelayLineManager
{
public:
    static DelayLineManager& getInstance()
    {
        static DelayLineManager instance;
        return instance;
    }

    void allocate(const std::string& name, double maxDurationMs, double sampleRate);
    void writeSample(const std::string& name, float sampleL, float sampleR);
    void writeBlock(const std::string& name, const juce::AudioBuffer<float>& buffer, int numSamples);
    
    // Read with 4-point Hermite cubic interpolation
    void readInterpolated(const std::string& name, double delaySamples, float& outL, float& outR) const;
    void readBlock(const std::string& name, double delaySamples, juce::AudioBuffer<float>& outBuffer, int numSamples) const;

    bool hasLine(const std::string& name) const;
    void clearAll();

private:
    DelayLineManager() = default;

    struct DelayBuffer
    {
        std::vector<float> bufferL;
        std::vector<float> bufferR;
        size_t writeIdx = 0;
        size_t size = 0;
        double sampleRate = 96000.0;
    };

    mutable std::mutex mutex;
    std::unordered_map<std::string, DelayBuffer> delayLines;
};


// ============================================================================
// DelwriteTildeNode ([delwrite~ <name> <max_ms>])
// ============================================================================

class DelwriteTildeNode : public RelativisticNode
{
public:
    DelwriteTildeNode(int id, const std::string& name = "del1", double maxDurationMs = 1000.0);
    ~DelwriteTildeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    const std::string& getDelayName() const { return delayName; }
    double getMaxDurationMs() const { return maxDurationMs; }

private:
    std::string delayName;
    double maxDurationMs;
};


// ============================================================================
// DelreadTildeNode ([delread~ <name> <delay_ms>])
// ============================================================================

class DelreadTildeNode : public RelativisticNode
{
public:
    DelreadTildeNode(int id, const std::string& name = "del1", double delayMs = 100.0);
    ~DelreadTildeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setDelayMs(double ms) { delayMs = std::max(0.0, ms); }
    double getDelayMs() const { return delayMs; }

private:
    std::string delayName;
    double delayMs;
};


// ============================================================================
// VdTildeNode ([vd~ <name>], [time.vd~ <name>]) - Variable Doppler Delay Reader
// ============================================================================

class VdTildeNode : public RelativisticNode
{
public:
    VdTildeNode(int id, const std::string& name = "del1", double defaultDelayMs = 100.0);
    ~VdTildeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setBaseDelayMs(double ms) { baseDelayMs = std::max(0.0, ms); }
    double getBaseDelayMs() const { return baseDelayMs; }

private:
    std::string delayName;
    double baseDelayMs = 100.0;
    double smoothedDelayMs = 100.0;
};


// ============================================================================
// PipeNode ([pipe], [time.pipe]) - Relativistic Control Message Delay Line
// ============================================================================

class PipeNode : public RelativisticNode
{
public:
    PipeNode(int id, double defaultDelayMs = 100.0);
    ~PipeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setDelayMs(double ms) { defaultDelayMs = std::max(0.0, ms); }
    double getDelayMs() const { return defaultDelayMs; }
    void flush();
    void clear();

private:
    struct QueuedEvent
    {
        std::string payload;
        double remainingSamples; // Decays with gamma * dt
    };

    double defaultDelayMs;
    std::vector<QueuedEvent> eventQueue;
    std::mutex queueMutex;
};


// ============================================================================
// TimerNode ([timer], [time.timer]) - Relativistic Proper-Time Chronometer
// ============================================================================

class TimerNode : public RelativisticNode
{
public:
    TimerNode(int id);
    ~TimerNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void resetTimer();
    double measureElapsedProperTimeMs();
    double measureElapsedCoordinateTimeMs();

private:
    double accumulatedProperTimeSec = 0.0;
    double accumulatedCoordinateTimeSec = 0.0;
    double startProperTimeSec = 0.0;
    double startCoordinateTimeSec = 0.0;
    bool isRunning = false;
};


// ============================================================================
// SnapshotTildeNode ([snapshot~], [time.snapshot~]) - Audio/Time Signal Sampler
// ============================================================================

class SnapshotTildeNode : public RelativisticNode
{
public:
    SnapshotTildeNode(int id);
    ~SnapshotTildeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void triggerSample();

private:
    float lastSampleVal = 0.0f;
    double lastGamma = 1.0;
    double lastTau = 0.0;
};


// ============================================================================
// TimeQuantizeNode ([time.quantize]) - Proper-Time Grid Quantizer
// ============================================================================

class TimeQuantizeNode : public RelativisticNode
{
public:
    TimeQuantizeNode(int id, double divisionMs = 125.0);
    ~TimeQuantizeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setDivisionMs(double ms) { gridDivisionMs = std::max(1.0, ms); }
    double getDivisionMs() const { return gridDivisionMs; }

private:
    struct PendingMsg
    {
        std::string payload;
    };

    double gridDivisionMs;
    double currentProperTimeAcc = 0.0;
    std::vector<PendingMsg> pendingQueue;
    std::mutex queueMutex;
};

} // namespace TimeDilationDAW
