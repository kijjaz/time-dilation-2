#pragma once

#include "RelativisticNodeGraph.h"
#include <vector>
#include <string>
#include <atomic>
#include <algorithm>
#include <cmath>

namespace TimeDilationDAW
{

// ==============================================================================
// 1. EuclidSequencerNode ([seq.euclid k n], [euclid k n])
// Generates Bjorklund Euclidean rhythms distributed across k pulses in n steps.
// Clocked by proper time with rotation offset and relativistic swing.
// ==============================================================================
class EuclidSequencerNode : public RelativisticNode
{
public:
    EuclidSequencerNode(int id, int pulses = 3, int totalSteps = 8, int rotateOffset = 0);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    int getPulses() const { return pulses; }
    int getTotalSteps() const { return totalSteps; }
    int getRotation() const { return rotation; }
    float getSwing() const { return swing; }

    void setParams(int k, int n, int rot = 0, float sw = 0.0f);
    const std::vector<bool>& getPattern() const { return pattern; }

private:
    void generateBjorklundPattern();

    int pulses = 3;
    int totalSteps = 8;
    int rotation = 0;
    float swing = 0.0f; // 0.0 to 0.75
    int currentStep = 0;

    std::vector<bool> pattern;
    double accumulatedProperTime = 0.0;
    double stepDurationSec = 0.125; // Default 16th note at 120 BPM
};

// ==============================================================================
// 2. ArpNode ([seq.arp], [arp])
// Relativistic chord arpeggiator (Up, Down, PingPong, Random, AsPlayed)
// with dynamic octave range (1-4 octaves) and note duration coupled to proper time.
// ==============================================================================
class ArpNode : public RelativisticNode
{
public:
    enum class ArpMode { Up, Down, PingPong, Random, AsPlayed };

    ArpNode(int id, ArpMode mode = ArpMode::Up, int octaveRange = 2, double rateSec = 0.125);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setChordNotes(const std::vector<int>& notes);
    void setMode(ArpMode m) { mode = m; rebuildNoteList(); }
    void setOctaves(int oct) { octaves = std::clamp(oct, 1, 4); rebuildNoteList(); }
    void setRate(double sec) { rateSec = std::max(0.01, sec); }

    ArpMode getMode() const { return mode; }
    int getOctaves() const { return octaves; }
    double getRate() const { return rateSec; }

private:
    void rebuildNoteList();

    ArpMode mode = ArpMode::Up;
    int octaves = 2;
    double rateSec = 0.125;

    std::vector<int> inputNotes{ 48, 52, 55, 59 }; // Default Cmaj7
    std::vector<int> generatedNotes;
    int currentIndex = 0;
    bool pingPongDirectionUp = true;

    double accumulatedProperTime = 0.0;
};

// ==============================================================================
// 3. PolySeqNode ([seq.poly])
// Multi-lane polyrhythmic sequencer with independent step lengths per track.
// e.g. Lane 1 (3 steps), Lane 2 (4 steps), Lane 3 (5 steps)
// ==============================================================================
class PolySeqNode : public RelativisticNode
{
public:
    struct Lane
    {
        std::vector<int> pitches;
        int currentStep = 0;
    };

    PolySeqNode(int id);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setLane(int laneIdx, const std::vector<int>& pitches);
    const std::vector<Lane>& getLanes() const { return lanes; }

private:
    std::vector<Lane> lanes;
    double accumulatedProperTime = 0.0;
    double baseStepDuration = 0.125;
};

// ==============================================================================
// 4. TimelineAutomationNode ([auto~], [timeline.auto])
// Sample-accurate parameter automation envelope evaluator.
// Reads multi-breakpoint curves with linear or C2 Hermite interpolation.
// ==============================================================================
class TimelineAutomationNode : public RelativisticNode
{
public:
    struct Breakpoint
    {
        double timeSec = 0.0;
        float value = 0.0f;
    };

    TimelineAutomationNode(int id, float defaultValue = 0.0f);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void clearBreakpoints();
    void addBreakpoint(double timeSec, float value);
    float evaluateAt(double timeSec) const;

    const std::vector<Breakpoint>& getBreakpoints() const { return breakpoints; }

private:
    std::vector<Breakpoint> breakpoints;
    std::atomic<float> currentValue{ 0.0f };
    double accumulatedProperTime = 0.0;
};

} // namespace TimeDilationDAW
