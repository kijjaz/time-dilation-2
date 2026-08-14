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
    const auto& inFrame = getTimeInlet("timeIn");
    auto& outFrame = getTimeOutlet("timeOut");

    outFrame = inFrame;
    outFrame.masterGamma = inFrame.masterGamma * factor;

    outFrame.sampleGamma.resize(static_cast<size_t>(numSamples));
    const bool hasInSampleGamma = (inFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));
    for (int s = 0; s < numSamples; ++s)
    {
        float inG = hasInSampleGamma ? inFrame.sampleGamma[static_cast<size_t>(s)] : static_cast<float>(inFrame.masterGamma);
        outFrame.sampleGamma[static_cast<size_t>(s)] = inG * static_cast<float>(factor);
    }

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
    const auto& inFrame = getInletTimeFrame(0);
    auto& outFrame = getOutletTimeFrame(0);

    outFrame = inFrame;
    outFrame.masterGamma = -inFrame.masterGamma;

    outFrame.sampleGamma.resize(static_cast<size_t>(numSamples));
    const bool hasInSampleGamma = (inFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));
    for (int s = 0; s < numSamples; ++s)
    {
        float inG = hasInSampleGamma ? inFrame.sampleGamma[static_cast<size_t>(s)] : static_cast<float>(inFrame.masterGamma);
        outFrame.sampleGamma[static_cast<size_t>(s)] = -inG;
    }

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
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addInlet("timeIn", PortDataType::Time);     // Inlet 1: Relativistic Time Clock Input (Violet)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("note", PortDataType::Audio);     // Outlet 1: MIDI Note Audio Output (Cyan)
    addOutlet("gate", PortDataType::Audio);     // Outlet 2: Gate Trigger Audio Output (Cyan)

    std::string s = patternStr;
    if (s.rfind("notes ", 0) == 0) s = s.substr(6);
    std::stringstream ss(s);
    int n;
    while (ss >> n)
    {
        notes.push_back(n);
    }
    if (notes.empty())
    {
        notes = { 48, 55, 58, 60, 62, 65, 67, 70 };
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

    double stepDurationSec = (60.0 / std::max(20.0, bpm)) / 4.0; // 16th note step duration (125ms @ 120 BPM)
    double baseGamma = (std::abs(inFrame.masterGamma) > 0.0001) ? inFrame.masterGamma : 1.0;
    const bool hasAudioRateGamma = (inFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double curGamma = hasAudioRateGamma ? static_cast<double>(inFrame.sampleGamma[static_cast<size_t>(s)]) : baseGamma;
        accumulatedTime += (1.0 / currentSampleRate) * curGamma;
        if (accumulatedTime >= stepDurationSec)
        {
            accumulatedTime -= stepDurationSec;
            currentStep = (currentStep + 1) % static_cast<int>(notes.size());

            char noteMsg[32];
            std::snprintf(noteMsg, sizeof(noteMsg), "%d", notes[static_cast<size_t>(currentStep)]);
            emitMessageOnMsgOut(noteMsg);
        }

        int noteVal = notes[static_cast<size_t>(currentStep)];
        noteL[s] = static_cast<float>(noteVal);
        gateL[s] = 1.0f;

        pushTimeScopeSample(static_cast<float>(noteVal));
    }

    if (noteBuf.getNumChannels() > 1) noteBuf.copyFrom(1, 0, noteBuf, 0, 0, numSamples);
    if (gateBuf.getNumChannels() > 1) gateBuf.copyFrom(1, 0, gateBuf, 0, 0, numSamples);
}

void SeqNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    std::string s = message;
    std::stringstream ss(s);
    std::string cmd;
    ss >> cmd;

    if (cmd == "bpm" && ss >> bpm)
    {
        bpm = std::clamp(bpm, 20.0, 400.0);
    }
    else if (cmd == "step" || cmd == "next" || cmd == "bang" || cmd == "play")
    {
        currentStep = (currentStep + 1) % static_cast<int>(notes.size());
        char noteMsg[32];
        std::snprintf(noteMsg, sizeof(noteMsg), "%d", notes[static_cast<size_t>(currentStep)]);
        emitMessageOnMsgOut(noteMsg);
    }
    else if (cmd == "reset" || cmd == "0")
    {
        currentStep = 0;
        accumulatedTime = 0.0;
    }
    else
    {
        // Parse raw note list or "notes 48 55 58..."
        std::string listStr = (cmd == "notes" || cmd == "set") ? s.substr(s.find(' ') + 1) : s;
        std::stringstream noteSs(listStr);
        std::vector<int> newNotes;
        int n;
        while (noteSs >> n)
        {
            newNotes.push_back(n);
        }
        if (!newNotes.empty())
        {
            notes = newNotes;
            currentStep = 0;
            accumulatedTime = 0.0;
        }
    }
}


// ============================================================================
// MtofNode Implementation (mtof)
// ============================================================================

MtofNode::MtofNode(int id)
    : RelativisticNode(id, "mtof~", "mtof~")
{
    addInlet("msgIn", PortDataType::Message);  // Inlet 0: Message Input (Gold)
    addInlet("note~", PortDataType::Audio);    // Inlet 1: MIDI Note Audio Input (Cyan)
    addOutlet("msgOut", PortDataType::Message);// Outlet 0: Message Output (Gold)
    addOutlet("freq~", PortDataType::Audio);   // Outlet 1: Frequency Audio Output in Hz (Cyan)
}

void MtofNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void MtofNode::process(int numSamples)
{
    const auto& noteBuf = getInletBuffer(1);
    auto& freqBuf = getOutletBuffer(1);
    freqBuf.clear();

    const float* noteL = noteBuf.getReadPointer(0);
    float* freqL = freqBuf.getWritePointer(0);

    double cNote = currentNote.load();

    for (int s = 0; s < numSamples; ++s)
    {
        double m = (noteL && noteBuf.getNumChannels() > 0 && std::abs(noteL[s]) > 0.0001f) ? static_cast<double>(noteL[s]) : cNote;
        // MIDI Note to Frequency in Hz: f = 440 * 2^((m - 69)/12)
        double freq = 440.0 * std::pow(2.0, (m - 69.0) / 12.0);
        freqL[s] = static_cast<float>(freq);
    }

    if (freqBuf.getNumChannels() > 1)
    {
        freqBuf.copyFrom(1, 0, freqBuf, 0, 0, numSamples);
    }
}

void MtofNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    try
    {
        std::stringstream ss(message);
        std::string cmd;
        double val = 69.0;
        if (ss >> val)
        {
            currentNote.store(val);
        }
        else if (ss >> cmd >> val)
        {
            currentNote.store(val);
        }
        double freq = 440.0 * std::pow(2.0, (currentNote.load() - 69.0) / 12.0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.2f", freq);
        if (onMessageEmitted) onMessageEmitted(buf);
    }
    catch (...) {}
}

// ============================================================================
// FtomNode Implementation (ftom / ftom~)
// ============================================================================

FtomNode::FtomNode(int id)
    : RelativisticNode(id, "ftom~", "ftom~")
{
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addInlet("freq~", PortDataType::Audio);     // Inlet 1: Frequency Audio Input in Hz (Cyan)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("note~", PortDataType::Audio);    // Outlet 1: MIDI Note Audio Output (Cyan)
}

void FtomNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void FtomNode::process(int numSamples)
{
    const auto& freqBuf = getInletBuffer(1);
    auto& noteBuf = getOutletBuffer(1);
    noteBuf.clear();

    const float* freqL = freqBuf.getReadPointer(0);
    float* noteL = noteBuf.getWritePointer(0);

    double cFreq = currentFreq.load();

    for (int s = 0; s < numSamples; ++s)
    {
        double f = (freqL && freqBuf.getNumChannels() > 0 && freqL[s] > 1.0f) ? static_cast<double>(freqL[s]) : cFreq;
        // Frequency in Hz to MIDI Note: m = 69 + 12 * log2(f / 440)
        double note = 69.0 + 12.0 * std::log2(std::max(1e-5, f) / 440.0);
        noteL[s] = static_cast<float>(note);
    }

    if (noteBuf.getNumChannels() > 1)
    {
        noteBuf.copyFrom(1, 0, noteBuf, 0, 0, numSamples);
    }
}

void FtomNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    try
    {
        std::stringstream ss(message);
        double val = 440.0;
        if (ss >> val)
        {
            currentFreq.store(val);
        }
        double note = 69.0 + 12.0 * std::log2(std::max(1e-5, currentFreq.load()) / 440.0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.1f", note);
        if (onMessageEmitted) onMessageEmitted(buf);
    }
    catch (...) {}
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
    : RelativisticNode(id, "time.lfo", "time.lfo"), rateHz(lfoRateHz), depth(lfoDepth)
{
    addInlet("timeIn", PortDataType::Time);    // Inlet 0: Relativistic Time input (Royal Violet)
    addInlet("rate", PortDataType::Message);   // Inlet 1: Modulation Rate (Hz) (Gold Accent)
    addOutlet("timeOut", PortDataType::Time);  // Outlet 0: Modulated Relativistic Time Output (Royal Violet)
    
    displayType = ScopeDisplayType::TimeFrame;
    timeVarMode = TimeScopeVariable::SpeedGamma;
}

void TimeLFONode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    phase = 0.0;
}

void TimeLFONode::process(int numSamples)
{
    const auto& timeInFrame = getTimeInlet("timeIn");
    auto& timeOutFrame = getTimeOutlet("timeOut");

    timeOutFrame = timeInFrame; // Base time frame copy
    timeOutFrame.sampleGamma.resize(static_cast<size_t>(numSamples));

    double currentRate = rateHz;
    double baseGamma = (timeInFrame.masterGamma != 0.0) ? timeInFrame.masterGamma : 1.0;
    double phaseInc = (2.0 * 3.14159265358979323846 * currentRate * std::abs(baseGamma)) / currentSampleRate;
    double lfoVal = 0.0;
    double modulatedGamma = baseGamma;

    const bool hasInSampleGamma = (timeInFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        lfoVal = std::sin(phase);
        phase += phaseInc;
        if (phase >= 2.0 * 3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;

        double inG = hasInSampleGamma ? static_cast<double>(timeInFrame.sampleGamma[static_cast<size_t>(s)]) : baseGamma;
        modulatedGamma = inG * (1.0 + depth * lfoVal);

        timeOutFrame.sampleGamma[static_cast<size_t>(s)] = static_cast<float>(modulatedGamma);

        pushTimeScopeSample(static_cast<float>(modulatedGamma));
        pushTimeTauScopeSample(static_cast<float>(depth * lfoVal));
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
