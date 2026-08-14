#pragma once

#include "RelativisticNodeGraph.h"
#include <vector>
#include <string>

namespace TimeDilationDAW
{

// time.warp~ node
class TimeWarpNode : public RelativisticNode
{
public:
    TimeWarpNode(int id, double warpFactor = 2.0, double couplingFactor = 1.0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    double factor = 2.0;
    double coupling = 1.0; // 1.0 = Full Physics Integration, 0.0 = Decoupled Speed, 0.5 = Elastic Return
};

// time.retro~ node
class TimeRetroNode : public RelativisticNode
{
public:
    TimeRetroNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
};

// time.stasis~ node
class TimeStasisNode : public RelativisticNode
{
public:
    TimeStasisNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
};

// time.math~ node (Lorentz Velocity Addition)
class TimeMathNode : public RelativisticNode
{
public:
    TimeMathNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
};

// seq node
class SeqNode : public RelativisticNode
{
public:
    SeqNode(int id, const std::string& patternStr = "60 62 64 65 67 69 71 72");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    const std::vector<int>& getNotes() const { return notes; }
    void setNotes(const std::vector<int>& n) { notes = n; }
    double getBpm() const { return bpm; }
    void setBpm(double b) { bpm = b; }

private:
    std::vector<int> notes;
    int currentStep = 0;
    double bpm = 120.0;
    double accumulatedTime = 0.0;
};

// mtof / mtof~ node (MIDI Note to Frequency in Hz)
class MtofNode : public RelativisticNode
{
public:
    MtofNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    std::atomic<double> currentNote{ 69.0 };
};

// ftom / ftom~ node (Frequency in Hz to MIDI Note)
class FtomNode : public RelativisticNode
{
public:
    FtomNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    std::atomic<double> currentFreq{ 440.0 };
};

// transport~ global project timeline object
class TransportNode : public RelativisticNode
{
public:
    TransportNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    double getPlayheadTimeSec() const { return currentPlayheadSec; }
    void setPlayheadTimeSec(double t) { currentPlayheadSec = t; }

private:
    double currentPlayheadSec = 0.0;
    bool isRunning = true;
};

// time.lfo~ relativistic time clock modulation LFO
class TimeLFONode : public RelativisticNode
{
public:
    TimeLFONode(int id, double lfoRateHz = 0.5, double lfoDepth = 0.8);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    double rateHz = 0.5;
    double depth = 0.8;
    double phase = 0.0;
};

// time.scope~ dual-trace relativistic time oscilloscope (Speed gamma vs Offset tau)
class TimeScopeNode : public RelativisticNode
{
public:
    TimeScopeNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;

    const std::vector<float>& getSpeedBuffer() const { return speedBuffer; }
    const std::vector<float>& getOffsetBuffer() const { return offsetBuffer; }

private:
    std::vector<float> speedBuffer;
    std::vector<float> offsetBuffer;
    int bufferSize = 256;
    int writeIdx = 0;
};

} // namespace TimeDilationDAW
