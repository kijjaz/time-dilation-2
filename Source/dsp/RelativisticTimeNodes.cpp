#include "RelativisticTimeNodes.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace TimeDilationDAW
{

// ============================================================================
// 1. TimeConstNode ([time.const~ <gamma> <tau_offset>], [time.speed~ <gamma>])
// ============================================================================

TimeConstNode::TimeConstNode(int id, double targetGamma, double targetTauMs)
    : RelativisticNode(id, "time.const~", "time.const~ " + std::to_string(targetGamma)),
      staticGamma(targetGamma), staticTauOffsetMs(targetTauMs)
{
    addOutlet("timeOut", PortDataType::Time); // Outlet 0: Time Frame Output
}

void TimeConstNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    accumulatedProperTimeSec = 0.0;
}

void TimeConstNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    if (msgStr == "freeze")   { freeze(); return; }
    if (msgStr == "resume")   { resume(); return; }
    if (msgStr == "reverse")  { reverse(); return; }

    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { staticGamma = tokens[0].getDoubleValue(); isFrozen = false; } catch (...) {}
    }
    else if (tokens.size() >= 2)
    {
        juce::String cmd = tokens[0].toLowerCase();
        if (cmd == "speed" || cmd == "gamma")
        {
            staticGamma = tokens[1].getDoubleValue();
            isFrozen = false;
        }
        else if (cmd == "offset" || cmd == "tau")
        {
            staticTauOffsetMs = tokens[1].getDoubleValue();
        }
        else if (cmd == "step")
        {
            staticGamma += tokens[1].getDoubleValue();
        }
    }
}

void TimeConstNode::process(int numSamples)
{
    auto& timeFrame = getTimeOutlet("timeOut");
    timeFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    double effectiveGamma = isFrozen ? 0.0 : staticGamma;
    double dt = 1.0 / currentSampleRate;
    double tauOffsetSec = staticTauOffsetMs * 0.001;

    for (int s = 0; s < numSamples; ++s)
    {
        accumulatedProperTimeSec += effectiveGamma * dt;
        timeFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(effectiveGamma);
    }

    timeFrame.masterGamma = effectiveGamma;
    timeFrame.masterTau = (accumulatedProperTimeSec + tauOffsetSec) * 1000.0;

    pushTimeScopeSample(static_cast<float>(effectiveGamma));
    pushTimeTauScopeSample(static_cast<float>(timeFrame.masterTau));
}


// ============================================================================
// 2. TimeScaleNode ([time.scale~ <mult> <offset_ms>], [time.mul~ <factor>])
// ============================================================================

TimeScaleNode::TimeScaleNode(int id, double factor, double offsetMs)
    : RelativisticNode(id, "time.scale~", "time.scale~ " + std::to_string(factor)),
      multiplier(factor), offsetMs(offsetMs)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
}

void TimeScaleNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    accumulatedProperTimeSec = 0.0;
}

void TimeScaleNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    if (msgStr == "invert") { multiplier = -multiplier; return; }

    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { multiplier = tokens[0].getDoubleValue(); } catch (...) {}
    }
    else if (tokens.size() >= 2)
    {
        juce::String cmd = tokens[0].toLowerCase();
        if (cmd == "mult" || cmd == "factor" || cmd == "scale")
        {
            multiplier = tokens[1].getDoubleValue();
        }
        else if (cmd == "offset")
        {
            offsetMs = tokens[1].getDoubleValue();
        }
    }
}

void TimeScaleNode::process(int numSamples)
{
    const auto& inFrame = getTimeInlet("timeIn");
    auto& outFrame = getTimeOutlet("timeOut");

    outFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    const bool hasAudioRateGamma = (inFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));
    double dt = 1.0 / currentSampleRate;
    double offsetSec = offsetMs * 0.001;

    for (int s = 0; s < numSamples; ++s)
    {
        double inGamma = hasAudioRateGamma ? static_cast<double>(inFrame.sampleGamma[static_cast<size_t>(s)]) : inFrame.masterGamma;
        double scaledGamma = inGamma * multiplier;

        accumulatedProperTimeSec += scaledGamma * dt;
        outFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(scaledGamma);
    }

    outFrame.masterGamma = inFrame.masterGamma * multiplier;
    outFrame.masterTau = (accumulatedProperTimeSec + offsetSec) * 1000.0;

    pushTimeScopeSample(static_cast<float>(outFrame.masterGamma));
    pushTimeTauScopeSample(static_cast<float>(outFrame.masterTau));
}


// ============================================================================
// 3. TimeAddNode ([time.add~ <delta_gamma> <delta_tau_ms>])
// ============================================================================

TimeAddNode::TimeAddNode(int id, double deltaGamma, double deltaTauMs)
    : RelativisticNode(id, "time.add~", "time.add~ " + std::to_string(deltaGamma)),
      deltaGamma(deltaGamma), deltaTauMs(deltaTauMs)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Time Frame Output
}

void TimeAddNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    accumulatedProperTimeSec = 0.0;
}

void TimeAddNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { deltaGamma = tokens[0].getDoubleValue(); } catch (...) {}
    }
    else if (tokens.size() >= 2)
    {
        juce::String cmd = tokens[0].toLowerCase();
        if (cmd == "gamma" || cmd == "speed")
        {
            deltaGamma = tokens[1].getDoubleValue();
        }
        else if (cmd == "offset" || cmd == "tau")
        {
            deltaTauMs = tokens[1].getDoubleValue();
        }
    }
}

void TimeAddNode::process(int numSamples)
{
    const auto& inFrame = getTimeInlet("timeIn");
    auto& outFrame = getTimeOutlet("timeOut");

    outFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    const bool hasAudioRateGamma = (inFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));
    double dt = 1.0 / currentSampleRate;
    double offsetSec = deltaTauMs * 0.001;

    for (int s = 0; s < numSamples; ++s)
    {
        double inGamma = hasAudioRateGamma ? static_cast<double>(inFrame.sampleGamma[static_cast<size_t>(s)]) : inFrame.masterGamma;
        double shiftedGamma = inGamma + deltaGamma;

        accumulatedProperTimeSec += shiftedGamma * dt;
        outFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(shiftedGamma);
    }

    outFrame.masterGamma = inFrame.masterGamma + deltaGamma;
    outFrame.masterTau = (accumulatedProperTimeSec + offsetSec) * 1000.0;

    pushTimeScopeSample(static_cast<float>(outFrame.masterGamma));
    pushTimeTauScopeSample(static_cast<float>(outFrame.masterTau));
}


// ============================================================================
// 4. TimeCrossfadeNode ([time.crossfade~ <mix>], [time.xfade~])
// ============================================================================

TimeCrossfadeNode::TimeCrossfadeNode(int id, double initialMix)
    : RelativisticNode(id, "time.crossfade~", "time.crossfade~ " + std::to_string(initialMix)),
      mix(initialMix), smoothedMix(initialMix)
{
    addInlet("timeIn1", PortDataType::Time);  // Inlet 1: Time Frame Input A
    addInlet("timeIn2", PortDataType::Time);  // Inlet 2: Time Frame Input B
    addInlet("mixMod~", PortDataType::Audio); // Inlet 3: Audio Rate Mix Modulation
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Mixed Time Frame Output
}

void TimeCrossfadeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    smoothedMix = mix;
    accumulatedProperTimeSec = 0.0;
}

void TimeCrossfadeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { setMix(tokens[0].getDoubleValue()); } catch (...) {}
    }
    else if (tokens.size() >= 2 && (tokens[0] == "mix" || tokens[0] == "xfade"))
    {
        setMix(tokens[1].getDoubleValue());
    }
}

void TimeCrossfadeNode::process(int numSamples)
{
    const auto& frame1 = getTimeInlet("timeIn1");
    const auto& frame2 = getTimeInlet("timeIn2");
    const auto& modBuf = getInletBuffer(2);

    auto& outFrame = getTimeOutlet("timeOut");
    outFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    const float* modPtr = modBuf.getReadPointer(0);
    const bool hasMod = (modBuf.getMagnitude(0, numSamples) > 0.0001f);
    const bool hasAudioRateG1 = (frame1.sampleGamma.size() >= static_cast<size_t>(numSamples));
    const bool hasAudioRateG2 = (frame2.sampleGamma.size() >= static_cast<size_t>(numSamples));

    double dt = 1.0 / currentSampleRate;

    for (int s = 0; s < numSamples; ++s)
    {
        double curMix = hasMod ? std::clamp(static_cast<double>(modPtr[s]), 0.0, 1.0) : mix;
        smoothedMix += 0.005 * (curMix - smoothedMix);

        double g1 = hasAudioRateG1 ? static_cast<double>(frame1.sampleGamma[static_cast<size_t>(s)]) : frame1.masterGamma;
        double g2 = hasAudioRateG2 ? static_cast<double>(frame2.sampleGamma[static_cast<size_t>(s)]) : frame2.masterGamma;

        double blendedGamma = (1.0 - smoothedMix) * g1 + smoothedMix * g2;
        accumulatedProperTimeSec += blendedGamma * dt;

        outFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(blendedGamma);
    }

    outFrame.masterGamma = (1.0 - smoothedMix) * frame1.masterGamma + smoothedMix * frame2.masterGamma;
    outFrame.masterTau = (1.0 - smoothedMix) * frame1.masterTau + smoothedMix * frame2.masterTau;

    pushTimeScopeSample(static_cast<float>(outFrame.masterGamma));
    pushTimeTauScopeSample(static_cast<float>(outFrame.masterTau));
}


// ============================================================================
// 5. TimeCurveNode ([time.curve~ <gamma> <duration_ms>], [time.ramp~])
// ============================================================================

TimeCurveNode::TimeCurveNode(int id, double targetGamma, double durationMs)
    : RelativisticNode(id, "time.curve~", "time.curve~ " + std::to_string(targetGamma)),
      currentGamma(targetGamma), startGamma(targetGamma), targetGamma(targetGamma)
{
    addOutlet("timeOut", PortDataType::Time); // Outlet 0: Time Frame Output
    rampTotalSamples = std::max(1.0, durationMs * 0.001 * 96000.0);
    rampCurrentSample = rampTotalSamples;
}

void TimeCurveNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    accumulatedProperTimeSec = 0.0;
}

void TimeCurveNode::rampTo(double newTargetGamma, double durationMs)
{
    startGamma = currentGamma;
    targetGamma = newTargetGamma;
    rampTotalSamples = std::max(1.0, durationMs * 0.001 * currentSampleRate);
    rampCurrentSample = 0.0;
    isRamping = true;
}

void TimeCurveNode::tapeStop(double durationMs)
{
    rampTo(0.0, durationMs);
}

void TimeCurveNode::tapeStart(double newTargetGamma, double durationMs)
{
    rampTo(newTargetGamma, durationMs);
}

void TimeCurveNode::instantJump(double newTargetGamma)
{
    currentGamma = newTargetGamma;
    startGamma = newTargetGamma;
    targetGamma = newTargetGamma;
    isRamping = false;
}

void TimeCurveNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    if (msgStr == "stop" || msgStr == "brake") { tapeStop(600.0); return; }
    if (msgStr == "start")                     { tapeStart(1.0, 500.0); return; }

    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { rampTo(tokens[0].getDoubleValue(), 500.0); } catch (...) {}
    }
    else if (tokens.size() >= 2)
    {
        juce::String cmd = tokens[0].toLowerCase();
        if (cmd == "ramp" || cmd == "curve" || cmd == "to")
        {
            double g = tokens[1].getDoubleValue();
            double dur = (tokens.size() >= 3) ? tokens[2].getDoubleValue() : 500.0;
            rampTo(g, dur);
        }
        else if (cmd == "jump" || cmd == "set")
        {
            instantJump(tokens[1].getDoubleValue());
        }
        else if (cmd == "stop")
        {
            tapeStop(tokens[1].getDoubleValue());
        }
    }
}

void TimeCurveNode::process(int numSamples)
{
    auto& timeFrame = getTimeOutlet("timeOut");
    timeFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    double dt = 1.0 / currentSampleRate;

    for (int s = 0; s < numSamples; ++s)
    {
        if (isRamping)
        {
            rampCurrentSample += 1.0;
            double u = std::clamp(rampCurrentSample / rampTotalSamples, 0.0, 1.0);
            // C2-continuous Hermite Smoothstep: 3*u^2 - 2*u^3
            double hermiteWeight = u * u * (3.0 - 2.0 * u);
            currentGamma = startGamma + (targetGamma - startGamma) * hermiteWeight;

            if (rampCurrentSample >= rampTotalSamples)
            {
                currentGamma = targetGamma;
                isRamping = false;
            }
        }

        accumulatedProperTimeSec += currentGamma * dt;
        timeFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(currentGamma);
    }

    timeFrame.masterGamma = currentGamma;
    timeFrame.masterTau = accumulatedProperTimeSec * 1000.0;

    pushTimeScopeSample(static_cast<float>(currentGamma));
    pushTimeTauScopeSample(static_cast<float>(timeFrame.masterTau));
}


// ============================================================================
// 6. TimeChaosNode ([time.chaos~ <rate> <attractor>])
// ============================================================================

TimeChaosNode::TimeChaosNode(int id, double initialRate, AttractorType type)
    : RelativisticNode(id, "time.chaos~", "time.chaos~ " + std::to_string(initialRate)),
      rate(initialRate), attractorType(type)
{
    addOutlet("timeOut", PortDataType::Time); // Outlet 0: Time Frame Output
    resetState();
}

void TimeChaosNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    accumulatedProperTimeSec = 0.0;
}

void TimeChaosNode::resetState()
{
    x = 0.1;
    y = 0.0;
    z = (attractorType == AttractorType::Lorenz) ? 20.0 : 0.0;
}

void TimeChaosNode::rk4StepLorenz(double dt)
{
    const double sigma = 10.0;
    const double rho = 28.0;
    const double beta = 8.0 / 3.0;

    auto dx = [=](double, double yVal, double xVal) { return sigma * (yVal - xVal); };
    auto dy = [=](double xVal, double yVal, double zVal) { return xVal * (rho - zVal) - yVal; };
    auto dz = [=](double xVal, double yVal, double zVal) { return xVal * yVal - beta * zVal; };

    double k1x = dx(0, y, x);
    double k1y = dy(x, y, z);
    double k1z = dz(x, y, z);

    double k2x = dx(0, y + 0.5 * dt * k1y, x + 0.5 * dt * k1x);
    double k2y = dy(x + 0.5 * dt * k1x, y + 0.5 * dt * k1y, z + 0.5 * dt * k1z);
    double k2z = dz(x + 0.5 * dt * k1x, y + 0.5 * dt * k1y, z + 0.5 * dt * k1z);

    double k3x = dx(0, y + 0.5 * dt * k2y, x + 0.5 * dt * k2x);
    double k3y = dy(x + 0.5 * dt * k2x, y + 0.5 * dt * k2y, z + 0.5 * dt * k2z);
    double k3z = dz(x + 0.5 * dt * k2x, y + 0.5 * dt * k2y, z + 0.5 * dt * k2z);

    double k4x = dx(0, y + dt * k3y, x + dt * k3x);
    double k4y = dy(x + dt * k3x, y + dt * k3y, z + dt * k3z);
    double k4z = dz(x + dt * k3x, y + dt * k3y, z + dt * k3z);

    x += (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
    y += (dt / 6.0) * (k1y + 2.0 * k2y + 2.0 * k3y + k4y);
    z += (dt / 6.0) * (k1z + 2.0 * k2z + 2.0 * k3z + k4z);
}

void TimeChaosNode::rk4StepRossler(double dt)
{
    const double a = 0.2;
    const double b = 0.2;
    const double c = 5.7;

    auto dx = [](double, double yVal, double zVal) { return -yVal - zVal; };
    auto dy = [=](double xVal, double yVal, double) { return xVal + a * yVal; };
    auto dz = [=](double xVal, double, double zVal) { return b + zVal * (xVal - c); };

    double k1x = dx(0, y, z);
    double k1y = dy(x, y, 0);
    double k1z = dz(x, 0, z);

    double k2x = dx(0, y + 0.5 * dt * k1y, z + 0.5 * dt * k1z);
    double k2y = dy(x + 0.5 * dt * k1x, y + 0.5 * dt * k1y, 0);
    double k2z = dz(x + 0.5 * dt * k1x, 0, z + 0.5 * dt * k1z);

    double k3x = dx(0, y + 0.5 * dt * k2y, z + 0.5 * dt * k2z);
    double k3y = dy(x + 0.5 * dt * k2x, y + 0.5 * dt * k2y, 0);
    double k3z = dz(x + 0.5 * dt * k2x, 0, z + 0.5 * dt * k2z);

    double k4x = dx(0, y + dt * k3y, z + dt * k3z);
    double k4y = dy(x + dt * k3x, y + dt * k3y, 0);
    double k4z = dz(x + dt * k3x, 0, z + dt * k3z);

    x += (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
    y += (dt / 6.0) * (k1y + 2.0 * k2y + 2.0 * k3y + k4y);
    z += (dt / 6.0) * (k1z + 2.0 * k2z + 2.0 * k3z + k4z);
}

void TimeChaosNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    if (msgStr == "reset") { resetState(); return; }

    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { setRate(tokens[0].getDoubleValue()); } catch (...) {}
    }
    else if (tokens.size() >= 2)
    {
        juce::String cmd = tokens[0].toLowerCase();
        if (cmd == "rate" || cmd == "speed")
        {
            setRate(tokens[1].getDoubleValue());
        }
        else if (cmd == "chaos" || cmd == "depth")
        {
            setChaosDepth(tokens[1].getDoubleValue());
        }
        else if (cmd == "attractor")
        {
            if (tokens[1].toLowerCase() == "rossler") setAttractor(AttractorType::Rossler);
            else setAttractor(AttractorType::Lorenz);
        }
    }
}

void TimeChaosNode::process(int numSamples)
{
    auto& timeFrame = getTimeOutlet("timeOut");
    timeFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    double dt = (1.0 / currentSampleRate) * rate * 10.0;
    double realDt = 1.0 / currentSampleRate;

    for (int s = 0; s < numSamples; ++s)
    {
        if (attractorType == AttractorType::Lorenz)
            rk4StepLorenz(dt);
        else
            rk4StepRossler(dt);

        // Map normalized attractor coordinate into gamma around 1.0
        double mappedGamma = 1.0;
        if (attractorType == AttractorType::Lorenz)
        {
            mappedGamma = 1.0 + chaosDepth * (z - 25.0) / 15.0;
        }
        else
        {
            mappedGamma = 1.0 + chaosDepth * (x / 10.0);
        }

        mappedGamma = std::clamp(mappedGamma, 0.05, 8.0);
        accumulatedProperTimeSec += mappedGamma * realDt;

        timeFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(mappedGamma);
    }

    timeFrame.masterGamma = static_cast<double>(timeFrame.sampleGamma.back());
    timeFrame.masterTau = accumulatedProperTimeSec * 1000.0;

    pushTimeScopeSample(timeFrame.sampleGamma.back());
    pushTimeTauScopeSample(static_cast<float>(timeFrame.masterTau));
}


// ============================================================================
// 7. TimeGridQuantizeNode ([time.quantize~ <division_ms> <swing>], [time.grid~])
// ============================================================================

TimeGridQuantizeNode::TimeGridQuantizeNode(int id, double divisionMs, double swing)
    : RelativisticNode(id, "time.quantize~", "time.quantize~ " + std::to_string(static_cast<int>(divisionMs))),
      divisionMs(divisionMs), swing(swing)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Quantized Time Frame Output
}

void TimeGridQuantizeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    accumulatedProperTimeSec = 0.0;
}

void TimeGridQuantizeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msgStr = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msgStr, " ", "");

    if (tokens.size() == 1)
    {
        try { setDivisionMs(tokens[0].getDoubleValue()); } catch (...) {}
    }
    else if (tokens.size() >= 2)
    {
        juce::String cmd = tokens[0].toLowerCase();
        if (cmd == "div" || cmd == "division" || cmd == "grid")
        {
            setDivisionMs(tokens[1].getDoubleValue());
        }
        else if (cmd == "swing")
        {
            setSwing(tokens[1].getDoubleValue());
        }
    }
}

void TimeGridQuantizeNode::process(int numSamples)
{
    const auto& inFrame = getTimeInlet("timeIn");
    auto& outFrame = getTimeOutlet("timeOut");

    outFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    const bool hasAudioRateGamma = (inFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));
    double dt = 1.0 / currentSampleRate;
    double divSec = std::max(0.001, divisionMs * 0.001);

    for (int s = 0; s < numSamples; ++s)
    {
        double inGamma = hasAudioRateGamma ? static_cast<double>(inFrame.sampleGamma[static_cast<size_t>(s)]) : inFrame.masterGamma;
        accumulatedProperTimeSec += inGamma * dt;

        double stepIdx = std::floor(accumulatedProperTimeSec / divSec);
        bool isOddStep = (static_cast<long long>(stepIdx) % 2 != 0);
        double swingOffset = isOddStep ? (swing * 0.5 * divSec) : 0.0;

        double quantizedTau = stepIdx * divSec + swingOffset;

        outFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(inGamma);
        (void)quantizedTau;
    }

    outFrame.masterGamma = inFrame.masterGamma;
    outFrame.masterTau = std::floor(inFrame.masterTau / divisionMs) * divisionMs;

    pushTimeScopeSample(static_cast<float>(outFrame.masterGamma));
    pushTimeTauScopeSample(static_cast<float>(outFrame.masterTau));
}


// ============================================================================
// 8. TimeSplitNode ([time.split~])
// ============================================================================

TimeSplitNode::TimeSplitNode(int id)
    : RelativisticNode(id, "time.split~", "time.split~")
{
    addInlet("timeIn", PortDataType::Time);     // Inlet 1: Relativistic Time Input
    addOutlet("timeOut", PortDataType::Time);   // Outlet 1: Pass-Through Time Output
    addOutlet("gamma~", PortDataType::Audio);   // Outlet 2: Audio Rate Speed Gamma (Cyan)
    addOutlet("tau~", PortDataType::Audio);     // Outlet 3: Audio Rate Proper Time Tau (Cyan)
}

void TimeSplitNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TimeSplitNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    if (message == "bang" || message == "poll")
    {
        const auto& inFrame = getTimeInlet("timeIn");
        emitMessageOnMsgOut("gamma " + std::to_string(inFrame.masterGamma) + " tau " + std::to_string(inFrame.masterTau));
    }
}

void TimeSplitNode::process(int numSamples)
{
    const auto& inFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = inFrame;

    auto& gammaBuf = getAudioOutlet("gamma~");
    auto& tauBuf = getAudioOutlet("tau~");

    if (gammaBuf.getNumChannels() < 1 || gammaBuf.getNumSamples() < numSamples)
        gammaBuf.setSize(1, numSamples, false, false, true);
    if (tauBuf.getNumChannels() < 1 || tauBuf.getNumSamples() < numSamples)
        tauBuf.setSize(1, numSamples, false, false, true);

    float* gPtr = gammaBuf.getWritePointer(0);
    float* tPtr = tauBuf.getWritePointer(0);

    const bool hasAudioRate = (inFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        gPtr[s] = hasAudioRate ? inFrame.sampleGamma[static_cast<size_t>(s)] : static_cast<float>(inFrame.masterGamma);
        tPtr[s] = static_cast<float>(inFrame.masterTau * 0.001);
    }
}


// ============================================================================
// 9. TimeMergeNode ([time.merge~])
// ============================================================================

TimeMergeNode::TimeMergeNode(int id)
    : RelativisticNode(id, "time.merge~", "time.merge~")
{
    addInlet("gammaIn~", PortDataType::Audio); // Inlet 1: Speed Gamma Audio Signal
    addInlet("tauIn~", PortDataType::Audio);   // Inlet 2: Offset Tau Audio Signal
    addOutlet("timeOut", PortDataType::Time);  // Outlet 1: Synthesized Time Frame Output
}

void TimeMergeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    accumulatedProperTimeSec = 0.0;
}

void TimeMergeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
}

void TimeMergeNode::process(int numSamples)
{
    const auto& gammaBuf = getAudioInlet("gammaIn~");
    const auto& tauBuf = getAudioInlet("tauIn~");

    auto& outFrame = getTimeOutlet("timeOut");
    outFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    const float* gPtr = gammaBuf.getReadPointer(0);
    const float* tPtr = tauBuf.getReadPointer(0);

    const bool hasG = (gammaBuf.getNumChannels() > 0 && gammaBuf.getMagnitude(0, numSamples) > 0.0f);
    const bool hasT = (tauBuf.getNumChannels() > 0 && tauBuf.getMagnitude(0, numSamples) > 0.0f);

    double dt = 1.0 / currentSampleRate;

    for (int s = 0; s < numSamples; ++s)
    {
        double g = hasG ? static_cast<double>(gPtr[s]) : 1.0;
        double tOffset = hasT ? static_cast<double>(tPtr[s]) : 0.0;

        accumulatedProperTimeSec += g * dt;
        outFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(g);
        (void)tOffset;
    }

    outFrame.masterGamma = hasG ? static_cast<double>(gPtr[numSamples - 1]) : 1.0;
    outFrame.masterTau = accumulatedProperTimeSec * 1000.0;

    pushTimeScopeSample(static_cast<float>(outFrame.masterGamma));
    pushTimeTauScopeSample(static_cast<float>(outFrame.masterTau));
}

} // namespace TimeDilationDAW
