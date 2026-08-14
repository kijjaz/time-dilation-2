#include "RelativisticSoundNodes.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace TimeDilationDAW
{

// TableManager Implementation
TableManager::TableManager()
{
    // Initialize dedicated global sine table for wavetable synthesis and table readers
    std::vector<float> sineTable(GlobalSineTable::TABLE_SIZE);
    for (int i = 0; i < GlobalSineTable::TABLE_SIZE; ++i)
    {
        double angle = (static_cast<double>(i) / static_cast<double>(GlobalSineTable::TABLE_SIZE)) * 2.0 * 3.14159265358979323846;
        sineTable[static_cast<size_t>(i)] = static_cast<float>(std::sin(angle));
    }
    tables["sine"] = sineTable;
    tables["__sine__"] = sineTable;
}

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

// -----------------------------------------------------------------------------
// GlobalSineTable Implementation (Hermite Cubic Interpolation)
// -----------------------------------------------------------------------------
GlobalSineTable::GlobalSineTable()
{
    table.resize(TABLE_SIZE);
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        double angle = (static_cast<double>(i) / static_cast<double>(TABLE_SIZE)) * 2.0 * 3.14159265358979323846;
        table[static_cast<size_t>(i)] = static_cast<float>(std::sin(angle));
    }
}

const GlobalSineTable& GlobalSineTable::getInstance()
{
    static GlobalSineTable instance;
    return instance;
}

float GlobalSineTable::lookup(double p) const noexcept
{
    // Fast phase normalization: wrap to [0, 1)
    double normP = p - std::floor(p);
    double indexD = normP * static_cast<double>(TABLE_SIZE);
    int i0 = static_cast<int>(indexD);
    float frac = static_cast<float>(indexD - static_cast<double>(i0));

    int im1 = (i0 - 1) & TABLE_MASK;
    int i1  = (i0 + 1) & TABLE_MASK;
    int i2  = (i0 + 2) & TABLE_MASK;
    i0      = i0 & TABLE_MASK;

    float ym1 = table[static_cast<size_t>(im1)];
    float y0  = table[static_cast<size_t>(i0)];
    float y1  = table[static_cast<size_t>(i1)];
    float y2  = table[static_cast<size_t>(i2)];

    // 4-point, 3rd-order Hermite polynomial interpolation (SNR > 140 dB, 32-bit float bit-accurate)
    float c0 = y0;
    float c1 = 0.5f * (y1 - ym1);
    float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

// -----------------------------------------------------------------------------
// PolyBLEP & PolyBLAMP Band-Limited Step & Ramp Functions
// -----------------------------------------------------------------------------
inline float polyBlep(double t, double dt) noexcept
{
    if (dt <= 1.0e-9) return 0.0f;
    // 0 <= t < dt (just after discontinuity)
    if (t < dt)
    {
        double r = t / dt;
        return static_cast<float>(2.0 * r - r * r - 1.0);
    }
    // 1 - dt < t < 1 (just before discontinuity)
    if (t > 1.0 - dt)
    {
        double r = (t - 1.0) / dt;
        return static_cast<float>(2.0 * r + r * r + 1.0);
    }
    return 0.0f;
}

inline float polyBlamp(double t, double dt) noexcept
{
    if (dt <= 1.0e-9) return 0.0f;
    // 0 <= t < dt
    if (t < dt)
    {
        double r = t / dt;
        return static_cast<float>(dt * ((-1.0 / 3.0) * r * r * r + r * r - r + (1.0 / 3.0)));
    }
    // 1 - dt < t < 1
    if (t > 1.0 - dt)
    {
        double r = (t - 1.0) / dt;
        return static_cast<float>(dt * ((1.0 / 3.0) * r * r * r + r * r + r + (1.0 / 3.0)));
    }
    return 0.0f;
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

float OscNode::getSampleAtPhase(double p, double dt) const
{
    // Normalize phase to [0, 1)
    double normP = p - std::floor(p);

    if (waveformType == "saw" || waveformType == "sawtooth")
    {
        // 2 * phase - 1.0 with PolyBLEP step correction (-2.0 step at t=0)
        double naive = 2.0 * normP - 1.0;
        return static_cast<float>(naive - polyBlep(normP, dt));
    }
    else if (waveformType == "sqr" || waveformType == "square" || waveformType == "pulse")
    {
        // Square wave with PolyBLEP corrections at t=0 (+2.0 step) and t=0.5 (-2.0 step)
        double naive = (normP < 0.5) ? 1.0 : -1.0;
        double pShift = normP - 0.5;
        pShift = pShift - std::floor(pShift);
        return static_cast<float>(naive + polyBlep(normP, dt) - polyBlep(pShift, dt));
    }
    else if (waveformType == "tri" || waveformType == "triangle")
    {
        // Continuous triangle wave 4 * |phase - 0.5| - 1.0 with PolyBLAMP slope corrections (+8 at t=0, -8 at t=0.5)
        double naive = 4.0 * std::abs(normP - 0.5) - 1.0;
        double pShift = normP - 0.5;
        pShift = pShift - std::floor(pShift);
        return static_cast<float>(naive + 8.0 * polyBlamp(normP, dt) - 8.0 * polyBlamp(pShift, dt));
    }

    // High-performance CPU-friendly Sine Wavetable with 4-point Hermite interpolation (>140 dB SNR)
    return GlobalSineTable::getInstance().lookup(normP);
}

void OscNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    auto& outBuf = getAudioOutlet("out~");
    outBuf.clear();

    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame; // Propagate relativistic clock downstream!
    const auto& freqBuf = getAudioInlet("freq");

    float* outL = outBuf.getWritePointer(0);
    float* outR = outBuf.getWritePointer(1);

    const float* freqRead = freqBuf.getNumChannels() > 0 ? freqBuf.getReadPointer(0) : nullptr;
    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double currentFreq = (freqRead && freqBuf.getMagnitude(0, numSamples) > 0.0001f) ? static_cast<double>(freqRead[s]) : frequency;
        double currentGamma = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;

        // Continuous audio-rate Doppler phase step modulated by local gamma clock
        double phaseStep = (currentFreq / currentSampleRate) * currentGamma;
        double dt = std::min(0.5, std::abs(phaseStep));

        float val = getSampleAtPhase(phase, dt);

        phase += phaseStep;
        if (phase >= 1.0 || phase < 0.0)
            phase = phase - std::floor(phase);

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
    juce::ScopedNoDenormals noDenormals;

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

        // Subnormal / Denormal protection
        if (std::abs(s1) < 1.0e-15) s1 = 0.0;
        if (std::abs(s2) < 1.0e-15) s2 = 0.0;

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
    juce::ScopedNoDenormals noDenormals;

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

MessageNode::MessageNode(int id, const std::string& msgText)
    : RelativisticNode(id, "msg", msgText), messageText(msgText)
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
    if (onMessageEmitted) onMessageEmitted(messageText);
}

void MessageNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void MessageNode::receiveMessage(const std::string& msg)
{
    messageText = msg;
    setLabel(msg);
    triggerMessage();
}

// -----------------------------------------------------------------------------
// BangNode Implementation (bang / bng)
// -----------------------------------------------------------------------------
BangNode::BangNode(int id)
    : RelativisticNode(id, "bang", "bang")
{
    addInlet("in", PortDataType::Message);
    addOutlet("out", PortDataType::Message);
}

void BangNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    flashTimer.store(0.0);
}

void BangNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
    double cur = flashTimer.load();
    if (cur > 0.0) flashTimer.store(std::max(0.0, cur - 0.05));
}

void BangNode::triggerBang()
{
    flashTimer.store(1.0);
    if (onMessageEmitted) onMessageEmitted("bang");
}

bool BangNode::isFlashing() const
{
    return flashTimer.load() > 0.01;
}

void BangNode::receiveMessage(const std::string& message)
{
    juce::ignoreUnused(message);
    triggerBang();
}

// -----------------------------------------------------------------------------
// ToggleNode Implementation (toggle / tgl)
// -----------------------------------------------------------------------------
ToggleNode::ToggleNode(int id, bool initialState)
    : RelativisticNode(id, "toggle", initialState ? "toggle [X]" : "toggle [ ]"), state(initialState)
{
    addInlet("in", PortDataType::Message);
    addOutlet("out", PortDataType::Message);
}

void ToggleNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void ToggleNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void ToggleNode::toggleState()
{
    setState(!state.load());
}

void ToggleNode::setState(bool newState)
{
    state.store(newState);
    setLabel(newState ? "toggle [X]" : "toggle [ ]");
    if (onMessageEmitted) onMessageEmitted(newState ? "1" : "0");
}

void ToggleNode::receiveMessage(const std::string& message)
{
    std::string s = message;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "bang" || s == "toggle") toggleState();
    else if (s == "1" || s == "on" || s == "true") setState(true);
    else if (s == "0" || s == "off" || s == "false") setState(false);
    else
    {
        float val = std::stof(s);
        setState(val != 0.0f);
    }
}

// -----------------------------------------------------------------------------
// NumberNode Implementation (number / num)
// -----------------------------------------------------------------------------
NumberNode::NumberNode(int id, double val)
    : RelativisticNode(id, "number", std::to_string(val)), value(val)
{
    addInlet("in", PortDataType::Message);
    addOutlet("out", PortDataType::Message);
}

void NumberNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void NumberNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void NumberNode::setValue(double newVal)
{
    value.store(newVal);
    char buf[64];
    if (std::abs(newVal - std::round(newVal)) < 0.0001) std::snprintf(buf, sizeof(buf), "%.0f", newVal);
    else std::snprintf(buf, sizeof(buf), "%.2f", newVal);
    setLabel(buf);
    if (onMessageEmitted) onMessageEmitted(buf);
}

void NumberNode::receiveMessage(const std::string& message)
{
    try
    {
        double val = std::stod(message);
        setValue(val);
    }
    catch (...) {}
}

// -----------------------------------------------------------------------------
// SymbolNode Implementation (symbol / sym)
// -----------------------------------------------------------------------------
SymbolNode::SymbolNode(int id, const std::string& symText)
    : RelativisticNode(id, "symbol", symText), symbolText(symText)
{
    addInlet("in", PortDataType::Message);
    addOutlet("out", PortDataType::Message);
}

void SymbolNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void SymbolNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void SymbolNode::setSymbolText(const std::string& text)
{
    symbolText = text;
    setLabel(text);
    if (onMessageEmitted) onMessageEmitted(text);
}

std::string SymbolNode::getSymbolText() const
{
    return symbolText;
}

void SymbolNode::receiveMessage(const std::string& message)
{
    setSymbolText(message);
}

// -----------------------------------------------------------------------------
// RadioNode Implementation (radio / hradio / vradio)
// -----------------------------------------------------------------------------
RadioNode::RadioNode(int id, int numOpts, int initialIdx)
    : RelativisticNode(id, "radio", "radio"), numOptions(numOpts), selectedIdx(initialIdx)
{
    addInlet("in", PortDataType::Message);
    addOutlet("out", PortDataType::Message);
}

void RadioNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void RadioNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void RadioNode::selectOption(int index)
{
    if (index >= 0 && index < numOptions)
    {
        selectedIdx.store(index);
        if (onMessageEmitted) onMessageEmitted(std::to_string(index));
    }
}

void RadioNode::receiveMessage(const std::string& message)
{
    try
    {
        int idx = std::stoi(message);
        selectOption(idx);
    }
    catch (...) {}
}

// -----------------------------------------------------------------------------
// DisplayNode Implementation (display / print / disp)
// -----------------------------------------------------------------------------
DisplayNode::DisplayNode(int id)
    : RelativisticNode(id, "display", "disp: ---")
{
    addInlet("in", PortDataType::Message);
    addInlet("audioIn~", PortDataType::Audio);
    addOutlet("out", PortDataType::Message);
}

void DisplayNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void DisplayNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void DisplayNode::receiveMessage(const std::string& message)
{
    displayText = message;
    setLabel(message);
}

std::string DisplayNode::getDisplayText() const
{
    return displayText;
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
    setVolumeDb(-6.0f); // Default volume at -6.0 dB
}

void OutNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    rmsL.store(0.0f);
    rmsR.store(0.0f);
    peakL.store(0.0f);
    peakR.store(0.0f);
    clipL.store(false);
    clipR.store(false);
    clipHoldCountL = 0;
    clipHoldCountR = 0;
    waveformBuffer.assign(512, 0.0f);
    waveWriteIdx = 0;
}

void OutNode::process(int numSamples)
{
    const auto& inL = getInletBuffer(1); // Inlet 1: Audio Left (in1~, Cyan)
    const auto& inR = getInletBuffer(2); // Inlet 2: Audio Right (in2~, Cyan)

    float vol = getOutputVolume();

    // Calculate RMS and Peak levels scaled by Master Output Volume
    float rawL = (inL.getNumChannels() > 0 && numSamples > 0) ? (inL.getRMSLevel(0, 0, numSamples) * vol) : 0.0f;
    float rawR = (inR.getNumChannels() > 0 && numSamples > 0) ? (inR.getRMSLevel(0, 0, numSamples) * vol) : 0.0f;

    float magL = (inL.getNumChannels() > 0 && numSamples > 0) ? (inL.getMagnitude(0, 0, numSamples) * vol) : 0.0f;
    float magR = (inR.getNumChannels() > 0 && numSamples > 0) ? (inR.getMagnitude(0, 0, numSamples) * vol) : 0.0f;

    float prevL = rmsL.load();
    float prevR = rmsR.load();
    rmsL.store(std::max(rawL, prevL * 0.82f));
    rmsR.store(std::max(rawR, prevR * 0.82f));

    float prevPeakL = peakL.load();
    float prevPeakR = peakR.load();
    peakL.store(std::max(magL, prevPeakL * 0.88f));
    peakR.store(std::max(magR, prevPeakR * 0.88f));

    // Sample Peak 0 dBFS clipping indicator (> 1.0) with visual hold time
    if (magL >= 1.0f) clipHoldCountL = 25; // ~250ms hold
    else if (clipHoldCountL > 0) --clipHoldCountL;
    clipL.store(clipHoldCountL > 0);

    if (magR >= 1.0f) clipHoldCountR = 25;
    else if (clipHoldCountR > 0) --clipHoldCountR;
    clipR.store(clipHoldCountR > 0);

    const float* readPtr = nullptr;
    if (inL.getNumChannels() > 0 && rawL > 0.00001f) readPtr = inL.getReadPointer(0);
    else if (inR.getNumChannels() > 0 && rawR > 0.00001f) readPtr = inR.getReadPointer(0);

    // Populate Stereo Multi-Channel Pass-Through Buffer (Ch 0 = Left, Ch 1 = Right) scaled by Master Volume
    auto& outBuf = getOutletBuffer(1);
    if (outBuf.getNumChannels() < 2 || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(2, numSamples, false, false, true);
    }
    outBuf.clear();
    if (inL.getNumChannels() > 0 && numSamples > 0)
    {
        outBuf.copyFrom(0, 0, inL, 0, 0, numSamples);
        outBuf.applyGain(0, 0, numSamples, vol);
    }
    if (inR.getNumChannels() > 0 && numSamples > 0)
    {
        outBuf.copyFrom(1, 0, inR, 0, 0, numSamples);
        outBuf.applyGain(1, 0, numSamples, vol);
    }

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
    double rs = 2.0 * 0.1 * massM; // Schwarzschild-like ratio
    double redshiftFactor = std::sqrt(std::max(0.01, 1.0 - rs / std::max(0.1, radiusR)));
    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double currentGamma = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        double effectiveFreq = baseFreq * redshiftFactor * currentGamma;
        double phaseStep = (effectiveFreq / sRate);

        phase += phaseStep;
        double normP = phase - std::floor(phase);
        float val = GlobalSineTable::getInstance().lookup(normP);
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
    juce::ScopedNoDenormals noDenormals;

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

        // Subnormal / Denormal protection
        if (std::abs(s1) < 1.0e-15) s1 = 0.0;
        if (std::abs(s2) < 1.0e-15) s2 = 0.0;

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

// -----------------------------------------------------------------------------
// MeterNode Implementation
// -----------------------------------------------------------------------------
MeterNode::MeterNode(int id, MeterMode mode)
    : RelativisticNode(id, "meter~", "meter~"), meterMode(mode)
{
    addInlet("in1~", PortDataType::Audio);  // Inlet 1: Audio Input (Cyan)
    addOutlet("out~", PortDataType::Audio); // Outlet 1: Pass-Through Audio Output (Cyan)
}

std::string MeterNode::getMeterModeName() const
{
    switch (meterMode)
    {
        case MeterMode::Peak: return "PEAK";
        case MeterMode::RMS:  return "RMS";
        case MeterMode::LUFS: return "LUFS";
    }
    return "PEAK";
}

void MeterNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    lufsSampleRate = sampleRate;

    // K-weighting pre-filter & RLB filter setup for EBU R128 LUFS loudness
    preZ1 = 0.0; preZ2 = 0.0;
    rlhZ1 = 0.0; rlhZ2 = 0.0;
    lufsAccumulator = 0.0;
    lufsSampleCount = 0;

    levelDb.store(-100.0f);
    peakDb.store(-100.0f);
}

void MeterNode::process(int numSamples)
{
    const auto& inBuf = getInletBuffer(1);
    auto& outBuf = getOutletBuffer(2);

    int chans = inBuf.getNumChannels();
    if (chans == 0 || numSamples <= 0) return;

    // Pass-through audio output
    for (int ch = 0; ch < std::min(chans, outBuf.getNumChannels()); ++ch)
    {
        outBuf.copyFrom(ch, 0, inBuf, ch, 0, numSamples);
    }

    const float* ptr = inBuf.getReadPointer(0);

    float blockPeak = 0.0f;
    double blockSumSq = 0.0;

    for (int s = 0; s < numSamples; ++s)
    {
        float val = std::abs(ptr[s]);
        if (val > blockPeak) blockPeak = val;
        blockSumSq += static_cast<double>(ptr[s] * ptr[s]);
    }

    float currentPeakDb = (blockPeak <= 0.00001f) ? -100.0f : juce::Decibels::gainToDecibels(blockPeak, -100.0f);
    float prevPeak = peakDb.load();
    peakDb.store(std::max(currentPeakDb, prevPeak * 0.88f));

    float currentLevelDb = -100.0f;

    if (meterMode == MeterMode::Peak)
    {
        currentLevelDb = currentPeakDb;
    }
    else if (meterMode == MeterMode::RMS)
    {
        float rmsVal = static_cast<float>(std::sqrt(blockSumSq / static_cast<double>(numSamples)));
        currentLevelDb = (rmsVal <= 0.00001f) ? -100.0f : juce::Decibels::gainToDecibels(rmsVal, -100.0f);
    }
    else // MeterMode::LUFS
    {
        // Continuous K-weighted integrated loudness estimate
        for (int s = 0; s < numSamples; ++s)
        {
            double x = static_cast<double>(ptr[s]);
            // Simplified high-pass/shelf weighting
            double yPre = x - 0.85 * preZ1; preZ1 = yPre;
            double yRlh = yPre - 0.95 * rlhZ1; rlhZ1 = yRlh;
            lufsAccumulator += yRlh * yRlh;
            lufsSampleCount++;
        }

        if (lufsSampleCount >= static_cast<int>(lufsSampleRate * 0.3)) // 300ms window
        {
            double meanSq = lufsAccumulator / static_cast<double>(lufsSampleCount);
            currentLevelDb = static_cast<float>(-0.691 + 10.0 * std::log10(std::max(1e-10, meanSq)));
            lufsAccumulator *= 0.5;
            lufsSampleCount /= 2;
        }
        else
        {
            currentLevelDb = levelDb.load();
        }
    }

    float prevLevel = levelDb.load();
    levelDb.store(std::max(currentLevelDb, prevLevel * 0.85f));
}

void MeterNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    std::string s = message;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s.find("lufs") != std::string::npos)
    {
        setMeterMode(MeterMode::LUFS);
    }
    else if (s.find("rms") != std::string::npos)
    {
        setMeterMode(MeterMode::RMS);
    }
    else if (s.find("peak") != std::string::npos)
    {
        setMeterMode(MeterMode::Peak);
    }
}

// -----------------------------------------------------------------------------
// SpectrogramNode Implementation
// -----------------------------------------------------------------------------
SpectrogramNode::SpectrogramNode(int id)
    : RelativisticNode(id, "spectrogram~", "spectrogram~")
{
    addInlet("in1~", PortDataType::Audio);  // Inlet 1: Audio Input (Cyan)
    addOutlet("out~", PortDataType::Audio); // Outlet 1: Pass-Through Audio Output (Cyan)

    fifoBuffer.assign(fftSize, 0.0f);
    fftData.assign(fftSize * 2, 0.0f);
    spectrogramGrid.assign(historyLength * numBins, 0.0f);
}

void SpectrogramNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    currentSampleRate = sampleRate;
    fifoWriteIdx = 0;
    std::fill(spectrogramGrid.begin(), spectrogramGrid.end(), 0.0f);
    gridWritePos.store(0);
}

void SpectrogramNode::process(int numSamples)
{
    const auto& inBuf = getInletBuffer(1);
    auto& outBuf = getOutletBuffer(2);

    int chans = inBuf.getNumChannels();
    if (chans == 0 || numSamples <= 0) return;

    // Pass-through audio output
    for (int ch = 0; ch < std::min(chans, outBuf.getNumChannels()); ++ch)
    {
        outBuf.copyFrom(ch, 0, inBuf, ch, 0, numSamples);
    }

    if (isFrozen) return;

    const float* ptr = inBuf.getReadPointer(0);

    for (int s = 0; s < numSamples; ++s)
    {
        fifoBuffer[fifoWriteIdx] = ptr[s];
        fifoWriteIdx++;

        if (fifoWriteIdx >= static_cast<size_t>(fftSize))
        {
            fifoWriteIdx = 0;

            // Copy to FFT working array & apply Hann window
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifoBuffer.begin(), fifoBuffer.end(), fftData.begin());
            window.multiplyWithWindowingTable(fftData.data(), fftSize);

            // Compute frequency magnitudes
            fftEngine.performFrequencyOnlyForwardTransform(fftData.data());

            // Write 256 bin magnitudes into current grid slice
            int writeSlice = gridWritePos.load();
            size_t rowOffset = static_cast<size_t>(writeSlice) * static_cast<size_t>(numBins);

            for (int b = 0; b < numBins; ++b)
            {
                float mag = fftData[static_cast<size_t>(b)];
                // Logarithmic compression for smooth visual dynamic range
                float normVal = std::clamp(std::log10(1.0f + mag * 8.0f) * 0.75f, 0.0f, 1.0f);
                spectrogramGrid[rowOffset + static_cast<size_t>(b)] = normVal;
            }

            gridWritePos.store((writeSlice + 1) % historyLength);
        }
    }
}

void SpectrogramNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    std::string s = message;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "freeze") isFrozen = true;
    else if (s == "resume") isFrozen = false;
    else if (s == "clear") std::fill(spectrogramGrid.begin(), spectrogramGrid.end(), 0.0f);
}

// ============================================================================
// PackNode Implementation (pack~ / bundle~ / join~)
// ============================================================================
PackNode::PackNode(int id, int numChannels)
    : RelativisticNode(id, "pack~", "pack~ " + std::to_string(numChannels)), channelCount(std::max(1, numChannels))
{
    addInlet("msgIn", PortDataType::Message); // Inlet 0: Message Input (Gold)
    for (int ch = 0; ch < channelCount; ++ch)
    {
        addInlet("ch" + std::to_string(ch + 1) + "~", PortDataType::Audio); // Inlets 1..N: Mono Audio Inputs (Cyan)
    }
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("multi~", PortDataType::Audio);   // Outlet 1: Multi-Channel Audio Output (Cyan)
}

void PackNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void PackNode::process(int numSamples)
{
    auto& outBuf = getOutletBuffer(1);
    if (outBuf.getNumChannels() < channelCount || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(channelCount, numSamples, false, false, true);
    }
    outBuf.clear();

    for (int ch = 0; ch < channelCount; ++ch)
    {
        const auto& inBuf = getInletBuffer(ch + 1);
        if (inBuf.getNumChannels() > 0 && numSamples > 0)
        {
            outBuf.copyFrom(ch, 0, inBuf, 0, 0, numSamples);
        }
    }
}

void PackNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    if (onMessageEmitted) onMessageEmitted("channels " + std::to_string(channelCount));
}

// ============================================================================
// UnpackNode Implementation (unpack~ / unbundle~ / split~)
// ============================================================================
UnpackNode::UnpackNode(int id, int numChannels)
    : RelativisticNode(id, "unpack~", "unpack~ " + std::to_string(numChannels)), channelCount(std::max(1, numChannels))
{
    addInlet("msgIn", PortDataType::Message); // Inlet 0: Message Input (Gold)
    addInlet("multi~", PortDataType::Audio);  // Inlet 1: Multi-Channel Audio Input (Cyan)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    for (int ch = 0; ch < channelCount; ++ch)
    {
        addOutlet("ch" + std::to_string(ch + 1) + "~", PortDataType::Audio); // Outlets 1..N: Mono Audio Outlets (Cyan)
    }
}

void UnpackNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void UnpackNode::process(int numSamples)
{
    const auto& multiBuf = getInletBuffer(1);

    for (int ch = 0; ch < channelCount; ++ch)
    {
        auto& outBuf = getOutletBuffer(ch + 1);
        if (outBuf.getNumChannels() < 1 || outBuf.getNumSamples() < numSamples)
        {
            outBuf.setSize(1, numSamples, false, false, true);
        }
        outBuf.clear();

        if (multiBuf.getNumChannels() > ch && numSamples > 0)
        {
            outBuf.copyFrom(0, 0, multiBuf, ch, 0, numSamples);
        }
    }
}

void UnpackNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    if (onMessageEmitted) onMessageEmitted("channels " + std::to_string(channelCount));
}

// ============================================================================
// ReverbNode Implementation (reverb~ / freeverb~)
// ============================================================================
ReverbNode::ReverbNode(int id, float roomSize, float damping, float wetLevel)
    : RelativisticNode(id, "reverb~", "reverb~")
{
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addInlet("in1~", PortDataType::Audio);      // Inlet 1: Left Audio Input (Cyan)
    addInlet("in2~", PortDataType::Audio);      // Inlet 2: Right Audio Input (Cyan)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("out1~", PortDataType::Audio);    // Outlet 1: Left Reverb Output (Cyan)
    addOutlet("out2~", PortDataType::Audio);    // Outlet 2: Right Reverb Output (Cyan)

    reverbParams.roomSize = roomSize;
    reverbParams.damping = damping;
    reverbParams.wetLevel = wetLevel;
    reverbParams.dryLevel = 1.0f - wetLevel * 0.5f;
    reverbParams.width = 1.0f;
    reverbEngine.setParameters(reverbParams);
}

void ReverbNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate > 1.0 ? sampleRate : 96000.0;
    spec.maximumBlockSize = static_cast<juce::uint32>(std::max(64, samplesPerBlock));
    spec.numChannels = 2;
    reverbEngine.prepare(spec);
    reverbEngine.reset();
}

void ReverbNode::process(int numSamples)
{
    const auto& inL = getInletBuffer(1);
    const auto& inR = getInletBuffer(2);

    auto& outL = getOutletBuffer(1);
    auto& outR = getOutletBuffer(2);

    if (outL.getNumChannels() < 1 || outL.getNumSamples() < numSamples) outL.setSize(1, numSamples, false, false, true);
    if (outR.getNumChannels() < 1 || outR.getNumSamples() < numSamples) outR.setSize(1, numSamples, false, false, true);

    outL.clear();
    outR.clear();

    if (numSamples <= 0) return;

    juce::AudioBuffer<float> tempStereo(2, numSamples);
    tempStereo.clear();

    if (inL.getNumChannels() > 0) tempStereo.copyFrom(0, 0, inL, 0, 0, numSamples);
    if (inR.getNumChannels() > 0) tempStereo.copyFrom(1, 0, inR, 0, 0, numSamples);
    else if (inL.getNumChannels() > 0) tempStereo.copyFrom(1, 0, inL, 0, 0, numSamples);

    juce::dsp::AudioBlock<float> block(tempStereo);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverbEngine.process(context);

    outL.copyFrom(0, 0, tempStereo, 0, 0, numSamples);
    outR.copyFrom(0, 0, tempStereo, 1, 0, numSamples);
}

void ReverbNode::receiveMessage(const std::string& msg)
{
    RelativisticNode::receiveMessage(msg);
    std::stringstream ss(msg);
    std::string key;
    float val = 0.0f;
    if (ss >> key >> val)
    {
        if (key == "room" || key == "size") reverbParams.roomSize = std::clamp(val, 0.0f, 1.0f);
        else if (key == "damp" || key == "damping") reverbParams.damping = std::clamp(val, 0.0f, 1.0f);
        else if (key == "wet") reverbParams.wetLevel = std::clamp(val, 0.0f, 1.0f);
        else if (key == "dry") reverbParams.dryLevel = std::clamp(val, 0.0f, 1.0f);
        else if (key == "width") reverbParams.width = std::clamp(val, 0.0f, 1.0f);
        reverbEngine.setParameters(reverbParams);
    }
}

// ============================================================================
// NoiseNode Implementation (noise~) - White & Paul Kellet Pink Noise
// ============================================================================
NoiseNode::NoiseNode(int id, const std::string& mode)
    : RelativisticNode(id, "noise~", "noise~ " + mode), noiseMode(mode)
{
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("out~", PortDataType::Audio);     // Outlet 1: Noise Audio Output (Cyan)
}

void NoiseNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    pinkB0 = pinkB1 = pinkB2 = 0.0f;
}

void NoiseNode::process(int numSamples)
{
    auto& outBuf = getOutletBuffer(1);
    if (outBuf.getNumChannels() < 1 || outBuf.getNumSamples() < numSamples) outBuf.setSize(1, numSamples, false, false, true);
    float* out = outBuf.getWritePointer(0);

    for (int s = 0; s < numSamples; ++s)
    {
        float white = (random.nextFloat() * 2.0f - 1.0f);
        if (noiseMode == "pink")
        {
            // Paul Kellet's filtered pink noise approximation
            pinkB0 = 0.99765f * pinkB0 + white * 0.0990460f;
            pinkB1 = 0.96300f * pinkB1 + white * 0.2965164f;
            pinkB2 = 0.57000f * pinkB2 + white * 1.0526913f;
            float pink = (pinkB0 + pinkB1 + pinkB2 + white * 0.1848f) * 0.15f;
            out[s] = std::clamp(pink, -1.0f, 1.0f);
        }
        else
        {
            out[s] = white;
        }
    }
}

void NoiseNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    std::string s = message;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s.find("pink") != std::string::npos) noiseMode = "pink";
    else if (s.find("white") != std::string::npos) noiseMode = "white";
}

// ============================================================================
// KickNode Implementation (kick~ / drum.kick~)
// ============================================================================
KickNode::KickNode(int id, double basePitch, double decaySec)
    : RelativisticNode(id, "kick~", "kick~"), baseFreq(basePitch), decayTime(decaySec)
{
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addInlet("trig~", PortDataType::Audio);     // Inlet 1: Audio Trigger (Cyan)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("out~", PortDataType::Audio);     // Outlet 1: Kick Audio Output (Cyan)
}

void KickNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    envPhase = 1.0;
    oscPhase = 0.0;
}

void KickNode::trigger()
{
    envPhase = 0.0;
    oscPhase = 0.0;
}

void KickNode::process(int numSamples)
{
    auto& outBuf = getOutletBuffer(1);
    if (outBuf.getNumChannels() < 1 || outBuf.getNumSamples() < numSamples) outBuf.setSize(1, numSamples, false, false, true);
    float* out = outBuf.getWritePointer(0);

    const auto& trigBuf = getInletBuffer(1);
    if (trigBuf.getNumChannels() > 0 && numSamples > 0)
    {
        const float* trigIn = trigBuf.getReadPointer(0);
        for (int s = 0; s < numSamples; ++s)
        {
            if (trigIn[s] > 0.5f && lastTrigVal <= 0.5f) trigger();
            lastTrigVal = trigIn[s];
        }
    }

    if (isTriggered.exchange(false)) trigger();

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double dt = 1.0 / sRate;

    for (int s = 0; s < numSamples; ++s)
    {
        if (envPhase < 1.0)
        {
            double t = envPhase * decayTime;
            // Pitch Envelope: rapid drop from (baseFreq * 4.5) to baseFreq
            double pitchEnv = std::exp(-t * 28.0);
            double curFreq = baseFreq + baseFreq * 3.5 * pitchEnv;
            // Amplitude Envelope: punchy exponential decay
            double ampEnv = std::exp(-t * (4.5 / std::max(0.05, decayTime)));

            // Phase accumulation
            oscPhase += (curFreq / sRate);
            double normP = oscPhase - std::floor(oscPhase);
            float sample = GlobalSineTable::getInstance().lookup(normP);

            // Click transient
            float click = (t < 0.005) ? static_cast<float>(1.0 - t / 0.005) * 0.4f : 0.0f;

            out[s] = std::clamp(static_cast<float>(sample * ampEnv) + click, -1.0f, 1.0f);
            envPhase += dt / std::max(0.05, decayTime);
        }
        else
        {
            out[s] = 0.0f;
        }
    }
}

void KickNode::receiveMessage(const std::string& msg)
{
    RelativisticNode::receiveMessage(msg);
    std::string s = msg;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "bang" || s == "play" || s == "1" || s == "trig") isTriggered.store(true);
    else if (s.rfind("pitch ", 0) == 0 || s.rfind("freq ", 0) == 0) baseFreq = std::stod(s.substr(6));
    else if (s.rfind("decay ", 0) == 0) decayTime = std::stod(s.substr(6));
}

// ============================================================================
// SnareNode Implementation (snare~ / drum.snare~)
// ============================================================================
SnareNode::SnareNode(int id, double toneFreq, double snappy, double decaySec)
    : RelativisticNode(id, "snare~", "snare~"), toneFrequency(toneFreq), snappyAmount(snappy), decayTime(decaySec)
{
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addInlet("trig~", PortDataType::Audio);     // Inlet 1: Audio Trigger (Cyan)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("out~", PortDataType::Audio);     // Outlet 1: Snare Audio Output (Cyan)
}

void SnareNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    envPhase = 1.0;
    bodyPhase1 = 0.0;
    bodyPhase2 = 0.0;
    lastTrigVal = 0.0f;
}

void SnareNode::trigger()
{
    envPhase = 0.0;
    bodyPhase1 = 0.0;
    bodyPhase2 = 0.0;
}

void SnareNode::process(int numSamples)
{
    auto& outBuf = getOutletBuffer(1);
    if (outBuf.getNumChannels() < 1 || outBuf.getNumSamples() < numSamples) outBuf.setSize(1, numSamples, false, false, true);
    float* out = outBuf.getWritePointer(0);

    const auto& trigBuf = getInletBuffer(1);
    if (trigBuf.getNumChannels() > 0 && numSamples > 0)
    {
        const float* trigIn = trigBuf.getReadPointer(0);
        for (int s = 0; s < numSamples; ++s)
        {
            if (trigIn[s] > 0.5f && lastTrigVal <= 0.5f) trigger();
            lastTrigVal = trigIn[s];
        }
    }

    if (isTriggered.exchange(false)) trigger();

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double dt = 1.0 / sRate;

    for (int s = 0; s < numSamples; ++s)
    {
        if (envPhase < 1.0)
        {
            double t = envPhase * decayTime;
            // Snare Body dual-tone resonators (toneFrequency + higher overtone)
            double bodyEnv = std::exp(-t * 18.0);
            bodyPhase1 += (toneFrequency / sRate);
            bodyPhase2 += ((toneFrequency * 1.62) / sRate);

            float body1 = GlobalSineTable::getInstance().lookup(bodyPhase1 - std::floor(bodyPhase1));
            float body2 = GlobalSineTable::getInstance().lookup(bodyPhase2 - std::floor(bodyPhase2));
            float body = (body1 * 0.6f + body2 * 0.4f) * static_cast<float>(bodyEnv);

            // Snappy noise burst with crisp decay
            double noiseEnv = std::exp(-t * (8.0 / std::max(0.05, decayTime)));
            float white = (random.nextFloat() * 2.0f - 1.0f);
            float noise = white * static_cast<float>(noiseEnv * snappyAmount);

            out[s] = std::clamp((body * 0.5f + noise * 0.7f), -1.0f, 1.0f);
            envPhase += dt / std::max(0.05, decayTime);
        }
        else
        {
            out[s] = 0.0f;
        }
    }
}

void SnareNode::receiveMessage(const std::string& msg)
{
    RelativisticNode::receiveMessage(msg);
    std::string s = msg;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "bang" || s == "play" || s == "1" || s == "trig") isTriggered.store(true);
    else if (s.rfind("tone ", 0) == 0 || s.rfind("freq ", 0) == 0) toneFrequency = std::stod(s.substr(5));
    else if (s.rfind("snappy ", 0) == 0) snappyAmount = std::stod(s.substr(7));
    else if (s.rfind("decay ", 0) == 0) decayTime = std::stod(s.substr(6));
}

// ============================================================================
// HiHatNode Implementation (hihat~ / drum.hat~)
// ============================================================================
HiHatNode::HiHatNode(int id, double decaySec)
    : RelativisticNode(id, "hihat~", "hihat~"), decayTime(decaySec)
{
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addInlet("trig~", PortDataType::Audio);     // Inlet 1: Audio Trigger (Cyan)
    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Output (Gold)
    addOutlet("out~", PortDataType::Audio);     // Outlet 1: Hi-Hat Audio Output (Cyan)
}

void HiHatNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    envPhase = 1.0;
    lastTrigVal = 0.0f;
    std::fill(std::begin(phases), std::end(phases), 0.0);
}

void HiHatNode::trigger(double decay)
{
    decayTime = decay;
    envPhase = 0.0;
}

void HiHatNode::process(int numSamples)
{
    auto& outBuf = getOutletBuffer(1);
    if (outBuf.getNumChannels() < 1 || outBuf.getNumSamples() < numSamples) outBuf.setSize(1, numSamples, false, false, true);
    float* out = outBuf.getWritePointer(0);

    const auto& trigBuf = getInletBuffer(1);
    if (trigBuf.getNumChannels() > 0 && numSamples > 0)
    {
        const float* trigIn = trigBuf.getReadPointer(0);
        for (int s = 0; s < numSamples; ++s)
        {
            if (trigIn[s] > 0.5f && lastTrigVal <= 0.5f) trigger(decayTime);
            lastTrigVal = trigIn[s];
        }
    }

    if (isTriggered.exchange(false)) trigger(decayTime);

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double dt = 1.0 / sRate;

    for (int s = 0; s < numSamples; ++s)
    {
        if (envPhase < 1.0)
        {
            double t = envPhase * decayTime;
            // Metallic 6-oscillator cluster
            float cluster = 0.0f;
            for (int i = 0; i < 6; ++i)
            {
                phases[i] += (freqs[i] / sRate);
                double normP = phases[i] - std::floor(phases[i]);
                cluster += (normP < 0.5 ? 0.16f : -0.16f);
            }

            // Exponential amplitude decay
            double ampEnv = std::exp(-t * (18.0 / std::max(0.02, decayTime)));
            out[s] = cluster * static_cast<float>(ampEnv);
            envPhase += dt / std::max(0.02, decayTime);
        }
        else
        {
            out[s] = 0.0f;
        }
    }
}

void HiHatNode::receiveMessage(const std::string& msg)
{
    RelativisticNode::receiveMessage(msg);
    std::string s = msg;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "bang" || s == "play" || s == "1" || s == "trig" || s == "close")
    {
        decayTime = 0.08;
        isTriggered.store(true);
    }
    else if (s == "open")
    {
        decayTime = 0.35;
        isTriggered.store(true);
    }
    else if (s.rfind("decay ", 0) == 0)
    {
        decayTime = std::stod(s.substr(6));
    }
}

} // namespace TimeDilationDAW
