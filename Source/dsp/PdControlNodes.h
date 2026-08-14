#pragma once

#include "RelativisticNodeGraph.h"
#include <vector>
#include <string>
#include <sstream>
#include <random>

namespace TimeDilationDAW
{

// ============================================================================
// TriggerNode ([trigger], [t])
// Evaluates inputs and fires outlets from right to left (N-1 down to 0).
// Supported outlet types: 'b' (bang), 'f' (float), 's' (symbol), 'a' (anything), or constant values.
// ============================================================================
class TriggerNode : public RelativisticNode
{
public:
    explicit TriggerNode(int id, const std::vector<std::string>& types);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    const std::vector<std::string>& getTypes() const { return outletTypes; }

private:
    std::vector<std::string> outletTypes;
    void executeTrigger(const std::string& incomingVal);
};

// ============================================================================
// SelectNode ([select], [sel])
// Matches input values against target arguments.
// Matched outlet fires a bang; unmatched value passes out the rightmost outlet.
// ============================================================================
class SelectNode : public RelativisticNode
{
public:
    explicit SelectNode(int id, const std::vector<std::string>& targets);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    const std::vector<std::string>& getTargets() const { return matchTargets; }

private:
    std::vector<std::string> matchTargets;
    void testMatch(const std::string& incomingVal);
};

// ============================================================================
// RouteNode ([route])
// Routes messages by matching their leading token / selector.
// Sends remaining arguments out corresponding outlet, or whole message out rightmost if no match.
// ============================================================================
class RouteNode : public RelativisticNode
{
public:
    explicit RouteNode(int id, const std::vector<std::string>& selectors);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    const std::vector<std::string>& getSelectors() const { return selectorKeys; }

private:
    std::vector<std::string> selectorKeys;
    void executeRoute(const std::string& incomingMsg);
};

// ============================================================================
// LineTildeNode ([line~])
// Relativistic linear audio-rate ramp generator.
// Modulated by local time dilation gamma clock.
// Syntax: "<target> <time_ms>" or lists like "0, 1 100 0 200"
// ============================================================================
class LineTildeNode : public RelativisticNode
{
public:
    explicit LineTildeNode(int id, double initialValue = 0.0);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    double getCurrentValue() const { return currentValue; }
    void setTarget(double target, double timeMs);

private:
    struct Segment
    {
        double targetValue = 0.0;
        double durationMs = 0.0;
    };

    double currentValue = 0.0;
    double targetValue = 0.0;
    double stepPerSample = 0.0;
    int samplesRemaining = 0;
    std::vector<Segment> segmentQueue;

    void processNextSegment();
};

// ============================================================================
// MetroNode ([metro])
// Relativistic control-rate metronome that sends bangs at specified intervals.
// Interval scales dynamically with proper time tau / gamma.
// ============================================================================
class MetroNode : public RelativisticNode
{
public:
    explicit MetroNode(int id, double intervalMs = 500.0, bool autoStart = false);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void start() { isRunning = true; sampleAccumulator = 0.0; }
    void stop() { isRunning = false; }
    void setIntervalMs(double ms) { intervalMsVal = std::max(1.0, ms); }
    double getIntervalMs() const { return intervalMsVal; }
    bool isActive() const { return isRunning; }

private:
    double intervalMsVal = 500.0;
    bool isRunning = false;
    double sampleAccumulator = 0.0;
};

// ============================================================================
// DelNode ([del], [delay])
// Relativistic control-rate delayed bang timer.
// Schedules a future bang scaled by local proper time.
// ============================================================================
class DelNode : public RelativisticNode
{
public:
    explicit DelNode(int id, double delayMs = 100.0);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void trigger(double delayMs);
    void stop() { isPending = false; }

private:
    double defaultDelayMs = 100.0;
    bool isPending = false;
    double sampleCountdown = 0.0;
};

// ============================================================================
// RandomNode ([random])
// Generates pseudo-random integer in [0, maxVal - 1] upon receiving bang.
// ============================================================================
class RandomNode : public RelativisticNode
{
public:
    explicit RandomNode(int id, int maxVal = 10);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setMax(int maxVal) { maxRange = std::max(1, maxVal); }
    int getMax() const { return maxRange; }

private:
    int maxRange = 10;
    std::mt19937 rng{ std::random_device{}() };
};

// ============================================================================
// CounterNode ([counter])
// Step counter / accumulator with min, max, step, and direction controls.
// ============================================================================
class CounterNode : public RelativisticNode
{
public:
    explicit CounterNode(int id, int minVal = 0, int maxVal = 16, int step = 1);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    int getCurrentCount() const { return currentVal; }
    void reset() { currentVal = minRange; }

private:
    int minRange = 0;
    int maxRange = 16;
    int stepSize = 1;
    int currentVal = 0;
    int direction = 1; // 1 = up, -1 = down
};

} // namespace TimeDilationDAW
