#include "RelativisticSoundNodes.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace TimeDilationDAW
{

// TableManager Singleton
TableManager& TableManager::getInstance()
{
    static TableManager instance;
    return instance;
}

void TableManager::createTable(const std::string& name, size_t sizeInSamples)
{
    tables[name] = std::vector<float>(sizeInSamples, 0.0f);
}

std::vector<float>& TableManager::getTable(const std::string& name)
{
    static std::vector<float> empty;
    auto it = tables.find(name);
    if (it != tables.end()) return it->second;
    return empty;
}

bool TableManager::hasTable(const std::string& name) const
{
    return tables.find(name) != tables.end();
}


// ============================================================================
// OscNode Implementation (osc~)
// ============================================================================

OscNode::OscNode(int id, const std::string& waveform)
    : RelativisticNode(id, "osc~", "osc~ " + waveform), waveformType(waveform)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time input
    addInlet("freq", PortDataType::Audio);    // Inlet 2: Frequency in Hz
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Relativistic Time output
    addOutlet("out~", PortDataType::Audio);   // Outlet 2: Audio Output
}

void OscNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    phase = 0.0;
}

float OscNode::getSampleAtPhase(double p) const
{
    // Normalize phase to [0, 1)
    double normP = p - std::floor(p);
    if (waveformType == "saw")
    {
        return static_cast<float>(2.0 * normP - 1.0);
    }
    else if (waveformType == "sqr" || waveformType == "square")
    {
        return normP < 0.5 ? 1.0f : -1.0f;
    }
    else if (waveformType == "tri" || waveformType == "triangle")
    {
        return static_cast<float>(4.0 * std::abs(normP - 0.5) - 1.0);
    }

    // Default Sine Wave
    return static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * normP));
}

void OscNode::process(int numSamples)
{
    auto& outBuf = getAudioOutlet("out~");
    outBuf.clear();

    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame; // Propagate relativistic clock downstream!
    const auto& freqBuf = getAudioInlet("freq");

    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    const float* freqRead = freqBuf.getNumChannels() > 0 ? freqBuf.getReadPointer(0) : nullptr;

    double gamma = timeFrame.masterGamma;

    for (int s = 0; s < numSamples; ++s)
    {
        double currentFreq = (freqRead && freqBuf.getMagnitude(0, numSamples) > 0.0001f) ? static_cast<double>(freqRead[s]) : frequency;
        
        // Doppler phase step modulated by local gamma clock
        double phaseStep = (currentFreq / currentSampleRate) * gamma;
        phase += phaseStep;

        float val = getSampleAtPhase(phase);

        outL[s] = val;
        outR[s] = val;
    }
}


// ============================================================================
// TableNode Implementation (table)
// ============================================================================

TableNode::TableNode(int id, const std::string& name, size_t size)
    : RelativisticNode(id, "table", "table " + name), tableName(name), tableSize(size)
{
    TableManager::getInstance().createTable(tableName, tableSize);

    // Initialize with standard sine wave for testing
    auto& tbl = TableManager::getInstance().getTable(tableName);
    for (size_t i = 0; i < tbl.size(); ++i)
    {
        tbl[i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * (double)i / (double)tbl.size()));
    }
}

void TableNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TableNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}


// ============================================================================
// TabReadTildeNode Implementation (tabread~)
// ============================================================================

TabReadTildeNode::TabReadTildeNode(int id, const std::string& name)
    : RelativisticNode(id, "tabread~", "tabread~ " + name), tableName(name)
{
    addInlet("index~", PortDataType::Audio);
    addOutlet("out~", PortDataType::Audio);
}

void TabReadTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TabReadTildeNode::process(int numSamples)
{
    auto& outBuf = getOutletBuffer(1);
    outBuf.clear();

    const auto& indexBuf = getInletBuffer(1);
    if (!TableManager::getInstance().hasTable(tableName)) return;

    const auto& tbl = TableManager::getInstance().getTable(tableName);
    if (tbl.empty()) return;

    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);
    const float* idxRead = indexBuf.getReadPointer(0);

    for (int s = 0; s < numSamples; ++s)
    {
        double pos = static_cast<double>(idxRead[s]);
        pos = std::fmod(pos, static_cast<double>(tbl.size()));
        if (pos < 0.0) pos += tbl.size();

        // 4-Point C1 Cubic Hermite Interpolation
        int i0 = static_cast<int>(std::floor(pos));
        double f = pos - i0;

        int im1 = (i0 - 1 + tbl.size()) % tbl.size();
        int i1 = (i0 + 1) % tbl.size();
        int i2 = (i0 + 2) % tbl.size();

        double ym1 = tbl[im1];
        double y0  = tbl[i0];
        double y1  = tbl[i1];
        double y2  = tbl[i2];

        double c0 = y0;
        double c1 = 0.5 * (y1 - ym1);
        double c2 = ym1 - 2.5 * y0 + 2.0 * y1 - 0.5 * y2;
        double c3 = 0.5 * (y2 - ym1) + 1.5 * (y0 - y1);

        float val = static_cast<float>(((c3 * f + c2) * f + c1) * f + c0);

        outL[s] = val;
        outR[s] = val;
    }
}


// ============================================================================
// SVFNode Implementation (svf~)
// ============================================================================

SVFNode::SVFNode(int id, double cutoff, double q)
    : RelativisticNode(id, "svf~", "svf~"), cutoffFreq(cutoff), Q(q)
{
    addInlet("in~", PortDataType::Audio);
    addInlet("cutoff", PortDataType::Audio);
    addOutlet("lowpass~", PortDataType::Audio);
    addOutlet("highpass~", PortDataType::Audio);
    addOutlet("bandpass~", PortDataType::Audio);
    addOutlet("notch~", PortDataType::Audio);
}

void SVFNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    s1 = 0.0;
    s2 = 0.0;
}

void SVFNode::process(int numSamples)
{
    const auto& inBuf = getInletBuffer(1);
    const auto& cutBuf = getInletBuffer(2);

    auto& lpBuf = getOutletBuffer(1);
    auto& hpBuf = getOutletBuffer(2);
    auto& bpBuf = getOutletBuffer(3);
    auto& brBuf = getOutletBuffer(4);

    lpBuf.clear(); hpBuf.clear(); bpBuf.clear(); brBuf.clear();

    const float* inL = inBuf.getReadPointer(0);
    const float* cutL = cutBuf.getNumChannels() > 0 ? cutBuf.getReadPointer(0) : nullptr;

    float* lpL = lpBuf.getWritePointer(0);
    float* hpL = hpBuf.getWritePointer(0);
    float* bpL = bpBuf.getWritePointer(0);
    float* brL = brBuf.getWritePointer(0);

    for (int s = 0; s < numSamples; ++s)
    {
        double currentCut = (cutL && cutBuf.getMagnitude(0, numSamples) > 0.0001f) ? static_cast<double>(cutL[s]) : cutoffFreq;
        currentCut = std::clamp(currentCut, 20.0, currentSampleRate * 0.49);

        // Zero-Delay Feedback Chamberlin SVF
        double g = std::tan(3.14159265358979323846 * currentCut / currentSampleRate);
        double k = 1.0 / std::max(0.1, Q);

        double x = static_cast<double>(inL[s]);
        double v3 = x - s2;
        double v1 = (s1 + g * v3) / (1.0 + g * (g + k));
        double v2 = s2 + g * v1;

        s1 = 2.0 * v1 - s1;
        s2 = 2.0 * v2 - s2;

        double lp = v2;
        double hp = x - k * v1 - v2;
        double bp = v1;
        double br = hp + lp;

        lpL[s] = static_cast<float>(lp);
        hpL[s] = static_cast<float>(hp);
        bpL[s] = static_cast<float>(bp);
        brL[s] = static_cast<float>(br);
    }

    // Copy left to right channel for stereo
    lpBuf.copyFrom(1, 0, lpBuf, 0, 0, numSamples);
    hpBuf.copyFrom(1, 0, hpBuf, 0, 0, numSamples);
    bpBuf.copyFrom(1, 0, bpBuf, 0, 0, numSamples);
    brBuf.copyFrom(1, 0, brBuf, 0, 0, numSamples);
}


// ============================================================================
// DelayNode Implementation (delay~)
// ============================================================================

DelayNode::DelayNode(int id, double maxDelaySec)
    : RelativisticNode(id, "delay~", "delay~")
{
    addInlet("in~", PortDataType::Audio);
    addInlet("delayTime", PortDataType::Audio);
    addOutlet("out~", PortDataType::Audio);

    size_t sz = static_cast<size_t>(maxDelaySec * 192000.0);
    buffer.resize(sz, 0.0f);
}

void DelayNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

void DelayNode::process(int numSamples)
{
    const auto& inBuf = getInletBuffer(1);
    auto& outBuf = getOutletBuffer(1);
    outBuf.clear();

    const float* inL = inBuf.getReadPointer(0);
    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    int maxSamps = static_cast<int>(buffer.size());

    for (int s = 0; s < numSamples; ++s)
    {
        double delaySamps = delaySeconds * currentSampleRate;
        double readPos = static_cast<double>(writeIndex) - delaySamps;
        while (readPos < 0.0) readPos += maxSamps;

        int r0 = static_cast<int>(std::floor(readPos)) % maxSamps;
        int r1 = (r0 + 1) % maxSamps;
        double frac = readPos - std::floor(readPos);

        float delayedSample = static_cast<float>((1.0 - frac) * buffer[r0] + frac * buffer[r1]);

        float input = inL[s];
        buffer[writeIndex] = input + static_cast<float>(feedback) * delayedSample;

        writeIndex = (writeIndex + 1) % maxSamps;

        outL[s] = delayedSample;
        outR[s] = delayedSample;
    }
}


// ============================================================================
// LadderNode Implementation (ladder~) - Moog 4-Pole VA Ladder Filter
// ============================================================================

LadderNode::LadderNode(int id, double cutoff, double resonance)
    : RelativisticNode(id, "ladder~", "ladder~"), cutoffFreq(cutoff), resonanceVal(resonance)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time input
    addInlet("in~", PortDataType::Audio);    // Inlet 2: Audio Input
    addInlet("cutoff", PortDataType::Audio); // Inlet 3: Cutoff Modulation
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Relativistic Time output
    addOutlet("out~", PortDataType::Audio);   // Outlet 2: Audio Output

    ladderFilter.setMode(juce::dsp::LadderFilterMode::LPF24);
}

void LadderNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    ladderFilter.prepare(spec);
    ladderFilter.setCutoffFrequencyHz(static_cast<float>(cutoffFreq));
    ladderFilter.setResonance(static_cast<float>(resonanceVal));
}

void LadderNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame; // Propagate time frame downstream!

    const auto& inBuf = getAudioInlet("in~");
    const auto& cutBuf = getAudioInlet("cutoff");
    auto& outBuf = getAudioOutlet("out~");

    outBuf.copyFrom(0, 0, inBuf, 0, 0, numSamples);
    outBuf.copyFrom(1, 0, inBuf, 1, 0, numSamples);

    double effectiveCutoff = cutoffFreq * timeFrame.masterGamma;
    if (cutBuf.getMagnitude(0, numSamples) > 0.0001f)
    {
        float avgCut = cutBuf.getMagnitude(0, numSamples);
        effectiveCutoff = static_cast<double>(avgCut) * timeFrame.masterGamma;
    }

    ladderFilter.setCutoffFrequencyHz(std::clamp(static_cast<float>(effectiveCutoff), 20.0f, static_cast<float>(currentSampleRate * 0.49)));

    juce::dsp::AudioBlock<float> block(outBuf);
    juce::dsp::ProcessContextReplacing<float> context(block);
    ladderFilter.process(context);
}


// ============================================================================
// DriveNode Implementation (drive~) - juce_dsp WaveShaper Saturation
// ============================================================================

DriveNode::DriveNode(int id, double driveAmount)
    : RelativisticNode(id, "drive~", "drive~"), drive(driveAmount)
{
    addInlet("timeIn", PortDataType::Time);   // Inlet 1: Relativistic Time input
    addInlet("in~", PortDataType::Audio);    // Inlet 2: Audio Input
    addOutlet("timeOut", PortDataType::Time); // Outlet 1: Relativistic Time output
    addOutlet("out~", PortDataType::Audio);   // Outlet 2: Audio Output

    waveShaper.functionToUse = [](float x) {
        return std::tanh(x);
    };
}

void DriveNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    waveShaper.prepare(spec);
}

void DriveNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame; // Propagate time frame downstream!

    const auto& inBuf = getAudioInlet("in~");
    auto& outBuf = getAudioOutlet("out~");

    outBuf.copyFrom(0, 0, inBuf, 0, 0, numSamples);
    outBuf.copyFrom(1, 0, inBuf, 1, 0, numSamples);

    // Apply drive gain scaling
    outBuf.applyGain(static_cast<float>(drive));

    juce::dsp::AudioBlock<float> block(outBuf);
    juce::dsp::ProcessContextReplacing<float> context(block);
    waveShaper.process(context);
}


// ============================================================================
// PluckNode Implementation (pluck~) - Karplus-Strong Physical Modeling String
// ============================================================================

PluckNode::PluckNode(int id, double basePitch)
    : RelativisticNode(id, "pluck~", "pluck~"), pitch(basePitch)
{
    addInlet("timeIn", PortDataType::Time);
    addInlet("trigger", PortDataType::Audio);
    addInlet("pitch", PortDataType::Audio);
    addOutlet("out~", PortDataType::Audio);

    ringBuffer.resize(44100, 0.0f);
}

void PluckNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0f);
}

void PluckNode::triggerPluck()
{
    size_t period = static_cast<size_t>(currentSampleRate / std::max(20.0, pitch));
    period = std::clamp(period, (size_t)2, ringBuffer.size() / 2);

    for (size_t i = 0; i < period; ++i)
    {
        // White noise burst
        ringBuffer[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
    writeIdx = period;
    readIdx = 0;
}

void PluckNode::process(int numSamples)
{
    const auto& timeFrame = getTimeInlet("timeIn");
    const auto& trigBuf = getAudioInlet("trigger");
    const auto& pitchBuf = getAudioInlet("pitch");
    auto& outBuf = getAudioOutlet("out~");
    outBuf.clear();

    if (pitchBuf.getMagnitude(0, numSamples) > 0.0001f)
    {
        pitch = static_cast<double>(pitchBuf.getMagnitude(0, numSamples));
    }

    if (trigBuf.getMagnitude(0, numSamples) > 0.5f)
    {
        triggerPluck();
    }

    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    size_t period = static_cast<size_t>(currentSampleRate / std::max(20.0, pitch));
    period = std::clamp(period, (size_t)2, ringBuffer.size() / 2);

    double gamma = timeFrame.masterGamma;

    for (int s = 0; s < numSamples; ++s)
    {
        size_t nextReadIdx = (readIdx + 1) % period;

        // Karplus-Strong lowpass feedback average
        float s1 = ringBuffer[readIdx];
        float s2 = ringBuffer[nextReadIdx];
        float newSample = static_cast<float>(dampFactor) * 0.5f * (s1 + s2);

        ringBuffer[readIdx] = newSample;

        // Advance read index scaled by relativistic gamma clock
        readIdx = (readIdx + static_cast<size_t>(std::max(1.0, std::abs(gamma)))) % period;

        outL[s] = newSample;
        outR[s] = newSample;
    }
}


// ============================================================================
// MessageNode Implementation (msg)
// ============================================================================

MessageNode::MessageNode(int id, const std::string& messageText)
    : RelativisticNode(id, "msg", messageText), messageText(messageText)
{
    addInlet("trigger", PortDataType::Message);
    addOutlet("msgOut", PortDataType::Message);
}

void MessageNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    messagePending = false;
}

void MessageNode::triggerMessage()
{
    messagePending = true;
}

void MessageNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void MessageNode::receiveMessage(const std::string& msg)
{
    messageText = msg;
    setLabel(msg);
}

// ============================================================================
// receiveMessage Overrides (Explicit Property Key & Getter Protocol)
// ============================================================================

void OscNode::receiveMessage(const std::string& msg)
{
    juce::String s(msg);
    auto tokens = juce::StringArray::fromTokens(s, " ", "");
    if (tokens.isEmpty()) return;

    juce::String key = tokens[0].toLowerCase();
    if (key == "set" && tokens.size() >= 2)
    {
        tokens.remove(0);
        key = tokens[0].toLowerCase();
    }

    if (key == "get")
    {
        std::string queryKey = (tokens.size() >= 2) ? tokens[1].toStdString() : "freq";
        if (queryKey == "freq") emitMessageOnMsgOut("freq " + std::to_string(frequency));
        else if (queryKey == "wave") emitMessageOnMsgOut("wave " + waveformType);
        else if (queryKey == "phase") emitMessageOnMsgOut("phase " + std::to_string(phase));
    }
    else if (key == "freq" && tokens.size() >= 2)
    {
        frequency = tokens[1].getDoubleValue();
    }
    else if ((key == "wave" || key == "shape") && tokens.size() >= 2)
    {
        waveformType = tokens[1].toStdString();
        setLabel("osc~ " + waveformType);
    }
    else if (key == "phase" && tokens.size() >= 2)
    {
        phase = tokens[1].getDoubleValue();
    }
    else if (key == "sin" || key == "saw" || key == "square" || key == "tri")
    {
        waveformType = key.toStdString();
        setLabel("osc~ " + waveformType);
    }
    else
    {
        double val = s.getDoubleValue();
        frequency = val;
    }
}

void LadderNode::receiveMessage(const std::string& msg)
{
    juce::String s(msg);
    auto tokens = juce::StringArray::fromTokens(s, " ", "");
    if (tokens.isEmpty()) return;

    juce::String key = tokens[0].toLowerCase();
    if (key == "set" && tokens.size() >= 2)
    {
        tokens.remove(0);
        key = tokens[0].toLowerCase();
    }

    if (key == "get")
    {
        std::string queryKey = (tokens.size() >= 2) ? tokens[1].toStdString() : "cutoff";
        if (queryKey == "cutoff" || queryKey == "freq") emitMessageOnMsgOut("cutoff " + std::to_string(cutoffFreq));
        else if (queryKey == "res" || queryKey == "q") emitMessageOnMsgOut("res " + std::to_string(resonanceVal));
    }
    else if ((key == "cutoff" || key == "freq") && tokens.size() >= 2)
    {
        cutoffFreq = tokens[1].getDoubleValue();
        ladderFilter.setCutoffFrequencyHz(static_cast<float>(std::abs(cutoffFreq)));
    }
    else if ((key == "res" || key == "q") && tokens.size() >= 2)
    {
        resonanceVal = tokens[1].getDoubleValue();
        ladderFilter.setResonance(static_cast<float>(resonanceVal));
    }
    else
    {
        double val = s.getDoubleValue();
        cutoffFreq = val;
        ladderFilter.setCutoffFrequencyHz(static_cast<float>(std::abs(cutoffFreq)));
    }
}

void DriveNode::receiveMessage(const std::string& msg)
{
    juce::String s(msg);
    auto tokens = juce::StringArray::fromTokens(s, " ", "");
    if (tokens.isEmpty()) return;

    juce::String key = tokens[0].toLowerCase();
    if (key == "set" && tokens.size() >= 2)
    {
        tokens.remove(0);
        key = tokens[0].toLowerCase();
    }

    if (key == "get")
    {
        emitMessageOnMsgOut("drive " + std::to_string(drive));
    }
    else if (key == "drive" && tokens.size() >= 2)
    {
        drive = tokens[1].getDoubleValue();
    }
    else
    {
        double val = s.getDoubleValue();
        drive = val;
    }
}

void PluckNode::receiveMessage(const std::string& msg)
{
    juce::String s(msg);
    auto tokens = juce::StringArray::fromTokens(s, " ", "");
    if (tokens.isEmpty()) return;

    juce::String key = tokens[0].toLowerCase();
    if (key == "play" || key == "trigger")
    {
        triggerPluck();
    }
    else if ((key == "freq" || key == "pitch" || key == "set") && tokens.size() >= 2)
    {
        pitch = tokens[1].getDoubleValue();
        triggerPluck();
    }
    else if (key == "damp" && tokens.size() >= 2)
    {
        dampFactor = std::clamp(tokens[1].getDoubleValue(), -0.999, 0.999);
    }
    else
    {
        double val = s.getDoubleValue();
        pitch = val;
        triggerPluck();
    }
}

// ============================================================================
// OutNode Implementation (out~)
// ============================================================================

OutNode::OutNode(int id)
    : RelativisticNode(id, "out~", "out~ master")
{
    addInlet("in1~", PortDataType::Audio); // Inlet 0: Audio Left Input (Cyan)
    addInlet("in2~", PortDataType::Audio); // Inlet 1: Audio Right Input (Cyan)
    addOutlet("out~", PortDataType::Audio); // Outlet 0: Audio Pass-Through (Cyan)
}

void OutNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    rmsL.store(0.0f);
    rmsR.store(0.0f);
    waveformBuffer.assign(512, 0.0f);
    waveWriteIdx = 0;
}

void OutNode::process(int numSamples)
{
    const auto& inL = getInletBuffer(1); // Inlet 1: Audio Left (in1~, Cyan)
    const auto& inR = getInletBuffer(2); // Inlet 2: Audio Right (in2~, Cyan)

    float rawL = (inL.getNumChannels() > 0 && numSamples > 0) ? inL.getRMSLevel(0, 0, numSamples) : 0.0f;
    float rawR = (inR.getNumChannels() > 0 && numSamples > 0) ? inR.getRMSLevel(0, 0, numSamples) : 0.0f;

    float rmsValL = rawL;
    float rmsValR = rawR;

    float prevL = rmsL.load();
    float prevR = rmsR.load();
    rmsL.store(std::max(rmsValL, prevL * 0.82f));
    rmsR.store(std::max(rmsValR, prevR * 0.82f));

    const float* readPtr = nullptr;
    if (inL.getNumChannels() > 0 && rawL > 0.00001f) readPtr = inL.getReadPointer(0);
    else if (inR.getNumChannels() > 0 && rawR > 0.00001f) readPtr = inR.getReadPointer(0);

    if (readPtr && numSamples > 0)
    {
        if (waveformBuffer.size() < 512) waveformBuffer.resize(512, 0.0f);
        size_t len = waveformBuffer.size();
        for (int i = 0; i < numSamples; ++i)
        {
            waveformBuffer[waveWriteIdx] = readPtr[i];
            waveWriteIdx = (waveWriteIdx + 1) % len;
        }
    }
}

void OutNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    std::string s = message;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s.find("stop") != std::string::npos || s == "0")
    {
        if (onPlaybackStateChanged) onPlaybackStateChanged(false);
    }
    else if (s.find("play") != std::string::npos || s.find("start") != std::string::npos || s == "1")
    {
        if (onPlaybackStateChanged) onPlaybackStateChanged(true);
    }
}

// ============================================================================
// GravRedshiftOscNode Implementation (grav.osc~)
// ============================================================================

GravRedshiftOscNode::GravRedshiftOscNode(int id, double mass, double radius)
    : RelativisticNode(id, "time.grav.osc~", "time.grav.osc~"), baseFreq(440.0), massM(mass), radiusR(radius), phase(0.0)
{
    addInlet("timeIn", PortDataType::Time);
    addInlet("freq", PortDataType::Audio);
    addOutlet("timeOut", PortDataType::Time);
    addOutlet("out~", PortDataType::Audio);
}

void GravRedshiftOscNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    phase = 0.0;
}

void GravRedshiftOscNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    if (message.rfind("mass ", 0) == 0) massM = std::stod(message.substr(5));
    else if (message.rfind("radius ", 0) == 0) radiusR = std::stod(message.substr(7));
    else if (message.rfind("freq ", 0) == 0) baseFreq = std::stod(message.substr(5));
}

void GravRedshiftOscNode::process(int numSamples)
{
    auto& outBuf = getAudioOutlet("out~");
    if (outBuf.getNumChannels() < 2 || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(2, numSamples, false, false, true);
    }
    outBuf.clear();

    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    // Gravitational Redshift Factor: z_factor = sqrt(1 - 2GM / (r * c^2))
    double rs = 2.0 * 0.1 * massM; // Schwarzsild-like ratio
    double redshiftFactor = std::sqrt(std::max(0.01, 1.0 - rs / std::max(0.1, radiusR)));
    double gamma = (timeFrame.masterGamma > 0.0001) ? timeFrame.masterGamma : 1.0;
    double effectiveFreq = baseFreq * redshiftFactor * gamma;

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double phaseStep = (effectiveFreq / sRate);

    for (int s = 0; s < numSamples; ++s)
    {
        phase += phaseStep;
        float val = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * phase));
        outL[s] = val;
        outR[s] = val;
    }
}


// ============================================================================
// LorentzWarpFilterNode Implementation (time.lorentz~)
// ============================================================================

LorentzWarpFilterNode::LorentzWarpFilterNode(int id, double cutoff, double relVelocity)
    : RelativisticNode(id, "time.lorentz~", "time.lorentz~"), cutoffFreq(cutoff), velocityV(relVelocity)
{
    addInlet("timeIn", PortDataType::Time);
    addInlet("in~", PortDataType::Audio);
    addOutlet("timeOut", PortDataType::Time);
    addOutlet("out~", PortDataType::Audio);
}

void LorentzWarpFilterNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    s1 = 0.0;
    s2 = 0.0;
}

void LorentzWarpFilterNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    if (message.rfind("cutoff ", 0) == 0) cutoffFreq = std::stod(message.substr(7));
    else if (message.rfind("velocity ", 0) == 0) velocityV = std::stod(message.substr(9));
}

void LorentzWarpFilterNode::process(int numSamples)
{
    auto& outBuf = getAudioOutlet("out~");
    const auto& inBuf = getAudioInlet("in~");
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    // Relativistic velocity addition: u_eff = (v + (gamma - 1)) / (1 + v*(gamma - 1))
    double g = timeFrame.masterGamma;
    double u_rel = std::min(0.99, std::max(0.0, (velocityV + (g - 1.0)*0.1) / (1.0 + velocityV * (g - 1.0)*0.1)));
    double warpedCutoff = cutoffFreq * (1.0 + u_rel * 2.0);

    double g_coeff = std::tan(3.14159265358979323846 * std::min(warpedCutoff, currentSampleRate * 0.45) / currentSampleRate);
    double k = 1.41421356;
    double a1 = 1.0 / (1.0 + g_coeff * (g_coeff + k));
    double a2 = g_coeff * a1;
    double a3 = g_coeff * a2;

    const float* inL = inBuf.getReadPointer(0);
    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        double x = inL[i];
        double v1 = a1 * (x - s1 * k - s2);
        double v2 = s1 + g_coeff * v1;
        s1 = v2 + g_coeff * v1;
        double v3 = s2 + g_coeff * v2;
        s2 = v3 + g_coeff * v2;

        outL[i] = static_cast<float>(v3);
        outR[i] = static_cast<float>(v3);
    }
}


// ============================================================================
// TachyonGranularNode Implementation (tachyon.grain~)
// ============================================================================

TachyonGranularNode::TachyonGranularNode(int id, double grainDurationMs)
    : RelativisticNode(id, "time.tachyon.grain~", "time.tachyon.grain~"), grainDurMs(grainDurationMs)
{
    addInlet("timeIn", PortDataType::Time);
    addInlet("in~", PortDataType::Audio);
    addOutlet("timeOut", PortDataType::Time);
    addOutlet("out~", PortDataType::Audio);
}

void TachyonGranularNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    sampleBuf.assign(static_cast<size_t>(sampleRate * 2.0), 0.0f);
    writeHead = 0;
    grainPhase = 0.0;
}

void TachyonGranularNode::process(int numSamples)
{
    auto& outBuf = getAudioOutlet("out~");
    const auto& inBuf = getAudioInlet("in~");
    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    if (sampleBuf.empty()) return;

    const float* inL = inBuf.getReadPointer(0);
    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    size_t bufLen = sampleBuf.size();
    double gamma = timeFrame.masterGamma;

    for (int i = 0; i < numSamples; ++i)
    {
        sampleBuf[writeHead] = inL[i];
        writeHead = (writeHead + 1) % bufLen;

        // Granular window pitch/time shift driven by relativistic gamma
        grainPhase += (1.0 / (currentSampleRate * (grainDurMs * 0.001))) * gamma;
        if (grainPhase >= 1.0) grainPhase -= 1.0;

        double win = 0.5 * (1.0 - std::cos(2.0 * 3.14159265358979323846 * grainPhase));
        size_t readPos = (writeHead + bufLen - static_cast<size_t>(grainPhase * 4410.0)) % bufLen;
        float val = static_cast<float>(sampleBuf[readPos] * win);

        outL[i] = val;
        outR[i] = val;
    }
}

} // namespace TimeDilationDAW
