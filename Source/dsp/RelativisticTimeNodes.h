#pragma once

#include "RelativisticNodeGraph.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace TimeDilationDAW
{

// ============================================================================
// 1. TimeConstNode ([time.const~ <gamma> <tau_offset>], [time.speed~ <gamma>])
// Static / Stepped Time Dilation Speed & Offset Generator
// ============================================================================
class TimeConstNode : public RelativisticNode
{
public:
    TimeConstNode(int id, double targetGamma = 1.0, double targetTauMs = 0.0);
    ~TimeConstNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setSpeed(double g) { staticGamma = g; }
    void setOffsetMs(double tauMs) { staticTauOffsetMs = tauMs; }
    void freeze() { isFrozen = true; }
    void resume() { isFrozen = false; }
    void reverse() { staticGamma = -staticGamma; }

private:
    double staticGamma = 1.0;
    double staticTauOffsetMs = 0.0;
    double accumulatedProperTimeSec = 0.0;
    bool isFrozen = false;
};

// ============================================================================
// 2. TimeScaleNode ([time.scale~ <mult> <offset_ms>], [time.mul~ <factor>])
// Relativistic Time Multiplier, Polyrhythmic Divider & Scalar Warper
// ============================================================================
class TimeScaleNode : public RelativisticNode
{
public:
    TimeScaleNode(int id, double factor = 2.0, double offsetMs = 0.0);
    ~TimeScaleNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setMultiplier(double m) { multiplier = m; }
    void setOffsetMs(double off) { offsetMs = off; }

private:
    double multiplier = 2.0;
    double offsetMs = 0.0;
    double accumulatedProperTimeSec = 0.0;
};

// ============================================================================
// 3. TimeAddNode ([time.add~ <delta_gamma> <delta_tau_ms>])
// Relativistic Time Offset & Bias Summer (Micro-timing swing & groove)
// ============================================================================
class TimeAddNode : public RelativisticNode
{
public:
    TimeAddNode(int id, double deltaGamma = 0.0, double deltaTauMs = 0.0);
    ~TimeAddNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setDeltaGamma(double dg) { deltaGamma = dg; }
    void setDeltaTauMs(double dt) { deltaTauMs = dt; }

private:
    double deltaGamma = 0.0;
    double deltaTauMs = 0.0;
    double accumulatedProperTimeSec = 0.0;
};

// ============================================================================
// 4. TimeCrossfadeNode ([time.crossfade~ <mix>], [time.xfade~ <mix>])
// Relativistic Time Frame Morph & Crossfader between two time streams
// ============================================================================
class TimeCrossfadeNode : public RelativisticNode
{
public:
    TimeCrossfadeNode(int id, double initialMix = 0.5);
    ~TimeCrossfadeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setMix(double m) { mix = std::clamp(m, 0.0, 1.0); }

private:
    double mix = 0.5;
    double smoothedMix = 0.5;
    double accumulatedProperTimeSec = 0.0;
};

// ============================================================================
// 5. TimeCurveNode ([time.curve~ <gamma> <duration_ms>], [time.ramp~])
// C2-Continuous Hermite S-Curve Time Accelerator, Decelerator & Tape Stop
// ============================================================================
class TimeCurveNode : public RelativisticNode
{
public:
    TimeCurveNode(int id, double targetGamma = 1.0, double durationMs = 1000.0);
    ~TimeCurveNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void rampTo(double targetGamma, double durationMs);
    void tapeStop(double durationMs = 800.0);
    void tapeStart(double targetGamma = 1.0, double durationMs = 600.0);
    void instantJump(double targetGamma);

private:
    double currentGamma = 1.0;
    double startGamma = 1.0;
    double targetGamma = 1.0;
    double rampTotalSamples = 1.0;
    double rampCurrentSample = 1.0;
    bool isRamping = false;

    double accumulatedProperTimeSec = 0.0;
};

// ============================================================================
// 6. TimeChaosNode ([time.chaos~ <rate> <attractor>])
// Strange Attractor 3D Organic Time Modulator (Lorenz & Rössler RK4)
// ============================================================================
class TimeChaosNode : public RelativisticNode
{
public:
    enum class AttractorType { Lorenz, Rossler };

    TimeChaosNode(int id, double rate = 0.5, AttractorType type = AttractorType::Lorenz);
    ~TimeChaosNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setRate(double r) { rate = std::clamp(r, 0.001, 10.0); }
    void setAttractor(AttractorType t) { attractorType = t; resetState(); }
    void setChaosDepth(double d) { chaosDepth = std::clamp(d, 0.0, 5.0); }
    void resetState();

private:
    double rate = 0.5;
    AttractorType attractorType = AttractorType::Lorenz;
    double chaosDepth = 1.0;

    double x = 0.1, y = 0.0, z = 0.0;
    double accumulatedProperTimeSec = 0.0;

    void rk4StepLorenz(double dt);
    void rk4StepRossler(double dt);
};

// ============================================================================
// 7. TimeGridQuantizeNode ([time.quantize~ <division_ms> <swing>], [time.grid~])
// Relativistic Continuous-to-Stepped Grid Quantizer with Swing
// ============================================================================
class TimeGridQuantizeNode : public RelativisticNode
{
public:
    TimeGridQuantizeNode(int id, double divisionMs = 125.0, double swing = 0.0);
    ~TimeGridQuantizeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setDivisionMs(double div) { divisionMs = std::max(1.0, div); }
    void setSwing(double sw) { swing = std::clamp(sw, -0.9, 0.9); }

private:
    double divisionMs = 125.0;
    double swing = 0.0;
    double accumulatedProperTimeSec = 0.0;
};

// ============================================================================
// 8. TimeSplitNode ([time.split~])
// Relativistic Time Demultiplexer & Signal Extractor (TimeFrame -> Audio Signals)
// ============================================================================
class TimeSplitNode : public RelativisticNode
{
public:
    TimeSplitNode(int id);
    ~TimeSplitNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
};

// ============================================================================
// 9. TimeMergeNode ([time.merge~])
// Relativistic Time Multiplexer / Audio-to-Time Bridge (Audio Signals -> TimeFrame)
// ============================================================================
class TimeMergeNode : public RelativisticNode
{
public:
    TimeMergeNode(int id);
    ~TimeMergeNode() override = default;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    double accumulatedProperTimeSec = 0.0;
};

} // namespace TimeDilationDAW
