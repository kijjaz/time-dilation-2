#include "RelativisticSequencers.h"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace TimeDilationDAW
{

// ============================================================================
// TimeWarpNode Implementation (time.warp)
// ============================================================================

TimeWarpNode::TimeWarpNode(int id, double warpFactor, double couplingFactor)
    : RelativisticNode(id, "time.warp", "time.warp " + std::to_string(warpFactor)), factor(warpFactor), coupling(couplingFactor)
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
}

void TimeWarpNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TimeWarpNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
    const auto& inFrame = getTimeInlet("timeIn");
    auto& outFrame = getTimeOutlet("timeOut");

    outFrame = inFrame;
    outFrame.masterGamma = inFrame.masterGamma * factor;

    for (auto& stream : outFrame.streams)
    {
        stream.gamma *= factor;
        stream.offsetCoupling = coupling;
    }
}

void TimeWarpNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    juce::String s(message);
    auto tokens = juce::StringArray::fromTokens(s, " ", "");
    if (tokens.size() >= 2 && (tokens[0] == "set" || tokens[0] == "factor"))
    {
        factor = tokens[1].getDoubleValue();
        setLabel("time.warp " + std::to_string(factor));
    }
    else if (tokens.size() >= 2 && tokens[0] == "coupling")
    {
        coupling = std::clamp(tokens[1].getDoubleValue(), 0.0, 1.0);
    }
    else if (!tokens.isEmpty())
    {
        double val = tokens[0].getDoubleValue();
        if (val != 0.0)
        {
            factor = val;
            setLabel("time.warp " + std::to_string(factor));
        }
    }
}


// ============================================================================
// TimeRetroNode Implementation (time.retro)
// ============================================================================

TimeRetroNode::TimeRetroNode(int id)
    : RelativisticNode(id, "time.retro", "time.retro")
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
}

void TimeRetroNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TimeRetroNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
    const auto& inFrame = getInletTimeFrame(0);
    auto& outFrame = getOutletTimeFrame(0);

    outFrame = inFrame;
    outFrame.masterGamma = -inFrame.masterGamma;

    for (auto& stream : outFrame.streams)
    {
        stream.gamma = -stream.gamma;
    }
}


// ============================================================================
// TimeStasisNode Implementation (time.stasis)
// ============================================================================

TimeStasisNode::TimeStasisNode(int id)
    : RelativisticNode(id, "time.stasis", "time.stasis")
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
}

void TimeStasisNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TimeStasisNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
    const auto& inFrame = getInletTimeFrame(0);
    auto& outFrame = getOutletTimeFrame(0);

    outFrame = inFrame;
    outFrame.masterGamma = 0.0;

    for (auto& stream : outFrame.streams)
    {
        stream.gamma = 0.0;
    }
}


// ============================================================================
// TimeMathNode Implementation (time.math) - Lorentz Addition
// ============================================================================

TimeMathNode::TimeMathNode(int id)
    : RelativisticNode(id, "time.math", "time.math (lorentz)")
{
    addInlet("timeIn1", PortDataType::Time);
    addInlet("timeIn2", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
}

void TimeMathNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TimeMathNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
    const auto& inFrame1 = getInletTimeFrame(0);
    const auto& inFrame2 = getInletTimeFrame(1);
    auto& outFrame = getOutletTimeFrame(0);

    outFrame = inFrame1;

    double g1 = inFrame1.masterGamma;
    double g2 = inFrame2.masterGamma;

    // Relativistic Lorentz Composition Math: (g1 + g2) / (1 + g1 * g2)
    double denom = 1.0 + (g1 * g2);
    double combinedGamma = (std::abs(denom) < 1e-9) ? 0.0 : (g1 + g2) / denom;

    outFrame.masterGamma = combinedGamma;

    for (auto& stream : outFrame.streams)
    {
        stream.gamma = combinedGamma;
    }
}


// ============================================================================
// SeqNode Implementation (seq)
// ============================================================================

SeqNode::SeqNode(int id, const std::string& patternStr)
    : RelativisticNode(id, "seq", "seq " + patternStr)
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("note", PortDataType::Audio);
    addOutlet("gate", PortDataType::Audio);

    std::stringstream ss(patternStr);
    int n;
    while (ss >> n)
    {
        notes.push_back(n);
    }
    if (notes.empty())
    {
        notes = { 60, 62, 64, 65, 67, 69, 71, 72 };
    }
}

void SeqNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    currentStep = 0;
    accumulatedTime = 0.0;
}

void SeqNode::process(int numSamples)
{
    const auto& inFrame = getTimeInlet("timeIn");

    auto& noteBuf = getAudioOutlet("note");
    auto& gateBuf = getAudioOutlet("gate");

    noteBuf.clear();
    gateBuf.clear();

    float* noteL = noteBuf.getWritePointer(0);
    float* gateL = gateBuf.getWritePointer(0);

    double stepDurationSec = (60.0 / bpm) / 4.0; // 16th note step duration
    double dtPerSample = (1.0 / currentSampleRate) * inFrame.masterGamma;

    for (int s = 0; s < numSamples; ++s)
    {
        accumulatedTime += dtPerSample;
        if (accumulatedTime >= stepDurationSec)
        {
            accumulatedTime -= stepDurationSec;
            currentStep = (currentStep + 1) % static_cast<int>(notes.size());
        }

        int noteVal = notes[currentStep];
        noteL[s] = static_cast<float>(noteVal);
        gateL[s] = 1.0f;
    }

    noteBuf.copyFrom(1, 0, noteBuf, 0, 0, numSamples);
    gateBuf.copyFrom(1, 0, gateBuf, 0, 0, numSamples);
}

void SeqNode::receiveMessage(const std::string& message)
{
    std::stringstream ss(message);
    std::string cmd;
    ss >> cmd;
    if (cmd == "bpm" && ss >> bpm)
    {
        bpm = std::clamp(bpm, 20.0, 400.0);
    }
    else if ((cmd == "notes" || cmd == "set") && ss.good())
    {
        std::vector<int> newNotes;
        int n;
        while (ss >> n)
        {
            newNotes.push_back(n);
        }
        if (!newNotes.empty())
        {
            notes = newNotes;
            currentStep = 0;
        }
    }
}


// ============================================================================
// MtofNode Implementation (mtof)
// ============================================================================

MtofNode::MtofNode(int id)
    : RelativisticNode(id, "mtof", "mtof")
{
    addInlet("note", PortDataType::Audio);
    addOutlet("freq", PortDataType::Audio);
}

void MtofNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void MtofNode::process(int numSamples)
{
    const auto& noteBuf = getAudioInlet("note");
    auto& freqBuf = getAudioOutlet("freq");
    freqBuf.clear();

    const float* noteL = noteBuf.getReadPointer(0);
    float* freqL = freqBuf.getWritePointer(0);
    float* freqR = freqBuf.getWritePointer(1);

    for (int s = 0; s < numSamples; ++s)
    {
        double m = static_cast<double>(noteL[s]);
        // MIDI Note to Frequency in Hz: f = 440 * 2^((m - 69)/12)
        double freq = 440.0 * std::pow(2.0, (m - 69.0) / 12.0);

        freqL[s] = static_cast<float>(freq);
        freqR[s] = static_cast<float>(freq);
    }
}

// ============================================================================
// TransportNode Implementation (transport~)
// ============================================================================

TransportNode::TransportNode(int id)
    : RelativisticNode(id, "time.transport~", "time.transport~ master")
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 0: Relativistic Time input
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Timeline Automation Messages
    addOutlet("timeOut", PortDataType::Time);  // Outlet 1: Relativistic Master Timeline Clock
}

void TransportNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    currentPlayheadSec = 0.0;
}

void TransportNode::process(int numSamples)
{
    const auto& timeInFrame = getTimeInlet("timeIn");
    auto& timeOutFrame = getTimeOutlet("timeOut");
    timeOutFrame = timeInFrame; // Stream master timeline clock downstream!

    if (isRunning)
    {
        currentPlayheadSec += (static_cast<double>(numSamples) / currentSampleRate) * timeInFrame.masterGamma;
    }
}

void TransportNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    if (message == "stop" || message == "0") isRunning = false;
    else if (message == "play" || message == "start" || message == "1") isRunning = true;
    else if (message == "reset") currentPlayheadSec = 0.0;
}

// ============================================================================
// TimeLFONode Implementation (time.lfo~)
// ============================================================================

TimeLFONode::TimeLFONode(int id, double lfoRateHz, double lfoDepth)
    : RelativisticNode(id, "time.lfo~", "time.lfo~"), rateHz(lfoRateHz), depth(lfoDepth)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time input
    addInlet("rate", PortDataType::Audio);    // Inlet 2: Modulation Rate (Hz)
    addOutlet("timeOut", PortDataType::Time);  // Outlet 1: Modulated Relativistic Time Output
    addOutlet("mod~", PortDataType::Audio);   // Outlet 2: LFO Audio CV Output
}

void TimeLFONode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    phase = 0.0;
}

void TimeLFONode::process(int numSamples)
{
    const auto& timeInFrame = getTimeInlet("timeIn");
    const auto& rateBuf = getAudioInlet("rate");

    auto& timeOutFrame = getTimeOutlet("timeOut");
    auto& modBuf = getAudioOutlet("mod~");
    modBuf.clear();

    timeOutFrame = timeInFrame; // Base time frame copy

    float* modL = modBuf.getWritePointer(0);
    float* modR = modBuf.getWritePointer(1);

    double currentRate = rateHz;
    if (rateBuf.getMagnitude(0, numSamples) > 0.0001f)
    {
        currentRate = static_cast<double>(rateBuf.getMagnitude(0, numSamples));
    }

    double baseGamma = (timeInFrame.masterGamma != 0.0) ? timeInFrame.masterGamma : 1.0;
    double phaseInc = (2.0 * 3.14159265358979323846 * currentRate * std::abs(baseGamma)) / currentSampleRate;
    double lfoVal = 0.0;
    double modulatedGamma = baseGamma;

    for (int s = 0; s < numSamples; ++s)
    {
        lfoVal = std::sin(phase);
        phase += phaseInc;
        if (phase >= 2.0 * 3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;

        float cv = static_cast<float>(lfoVal);
        modL[s] = cv;
        modR[s] = cv;

        modulatedGamma = baseGamma * (1.0 + depth * lfoVal);
        pushScopeSample(static_cast<float>(modulatedGamma));
    }

    // Compound Modulate relativistic gamma: gamma(t) = baseGamma * (1.0 + depth * sin(wt))
    timeOutFrame.masterGamma = modulatedGamma;
    for (auto& stream : timeOutFrame.streams)
    {
        stream.gamma = stream.gamma * (1.0 + depth * lfoVal);
    }
}

void TimeLFONode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    juce::String s(message);
    auto tokens = juce::StringArray::fromTokens(s, " ", "");
    if (tokens.size() >= 2 && tokens[0] == "rate")
    {
        rateHz = std::max(0.001, tokens[1].getDoubleValue());
    }
    else if (tokens.size() >= 2 && tokens[0] == "depth")
    {
        depth = std::clamp(tokens[1].getDoubleValue(), 0.0, 5.0);
    }
}

// ============================================================================
// TimeScopeNode Implementation (time.scope~)
// ============================================================================

TimeScopeNode::TimeScopeNode(int id)
    : RelativisticNode(id, "time.scope~", "time.scope~")
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Time Frame Input
    addOutlet("timeOut", PortDataType::Time);  // Outlet 1: Pass-through Time Frame

    speedBuffer.resize(bufferSize, 1.0f);
    offsetBuffer.resize(bufferSize, 0.0f);
}

void TimeScopeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    std::fill(speedBuffer.begin(), speedBuffer.end(), 1.0f);
    std::fill(offsetBuffer.begin(), offsetBuffer.end(), 0.0f);
    writeIdx = 0;
}

void TimeScopeNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
    const auto& timeInFrame = getTimeInlet("timeIn");
    auto& timeOutFrame = getTimeOutlet("timeOut");
    timeOutFrame = timeInFrame;

    double gamma = timeInFrame.masterGamma;
    double tau = 0.0;
    if (!timeInFrame.streams.empty())
    {
        tau = timeInFrame.streams[0].tau;
    }

    speedBuffer[writeIdx] = static_cast<float>(gamma);
    offsetBuffer[writeIdx] = static_cast<float>(tau);
    writeIdx = (writeIdx + 1) % bufferSize;
}

} // namespace TimeDilationDAW
