#pragma once

#include "RelativisticNodeGraph.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>

namespace TimeDilationDAW
{

// Global shared table memory pool for named tables
class TableManager
{
public:
    static TableManager& getInstance();
    void createTable(const std::string& name, size_t sizeInSamples);
    std::vector<float>& getTable(const std::string& name);
    bool hasTable(const std::string& name) const;

private:
    TableManager() = default;
    std::unordered_map<std::string, std::vector<float>> tables;
};

// osc~ node
class OscNode : public RelativisticNode
{
public:
    OscNode(int id, const std::string& waveform = "sin");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setPhase(double p) { phase = p; }
    void setWaveform(const std::string& w) { waveformType = w; }

private:
    std::string waveformType = "sin";
    double frequency = 440.0;
    double phase = 0.0;

    float getSampleAtPhase(double p) const;
};

// table node
class TableNode : public RelativisticNode
{
public:
    TableNode(int id, const std::string& tableName = "array1", size_t size = 44100);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;

private:
    std::string tableName;
    size_t tableSize;
};

// tabread~ node
class TabReadTildeNode : public RelativisticNode
{
public:
    TabReadTildeNode(int id, const std::string& tableName = "array1");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;

private:
    std::string tableName;
};

// svf~ node
class SVFNode : public RelativisticNode
{
public:
    SVFNode(int id, double cutoff = 1000.0, double q = 0.707);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;

private:
    double cutoffFreq = 1000.0;
    double Q = 0.707;
    double s1 = 0.0;
    double s2 = 0.0;
};

// delay~ node
class DelayNode : public RelativisticNode
{
public:
    DelayNode(int id, double maxDelaySec = 2.0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    double delaySeconds = 0.25;
    double feedback = 0.4;
};

// ladder~ node (juce_dsp Moog Ladder Filter)
class LadderNode : public RelativisticNode
{
public:
    LadderNode(int id, double cutoff = 1000.0, double resonance = 0.5);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    double cutoffFreq = 1000.0;
    double resonanceVal = 0.5;
    juce::dsp::LadderFilter<float> ladderFilter;
};

// drive~ node (juce_dsp WaveShaper Saturation)
class DriveNode : public RelativisticNode
{
public:
    DriveNode(int id, double driveAmount = 2.0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    double drive = 2.0;
    juce::dsp::WaveShaper<float> waveShaper;
};

// pluck~ node (Karplus-Strong physical modeling plucked string)
class PluckNode : public RelativisticNode
{
public:
    PluckNode(int id, double basePitch = 220.0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void triggerPluck();

private:
    double pitch = 220.0;
    std::vector<float> ringBuffer;
    size_t writeIdx = 0;
    size_t readIdx = 0;
    double dampFactor = 0.99;
};

// msg (Message) node
class MessageNode : public RelativisticNode
{
public:
    MessageNode(int id, const std::string& messageText = "play");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void triggerMessage();
    void receiveMessage(const std::string& message) override;

    std::string getMessageText() const { return messageText; }

private:
    std::string messageText;
    bool messagePending = false;
};

// OutNode
class OutNode : public RelativisticNode
{
public:
    OutNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    std::function<void(bool play)> onPlaybackStateChanged;

    float getRmsL() const { return rmsL.load(); }
    float getRmsR() const { return rmsR.load(); }
    const std::vector<float>& getWaveformBuffer() const { return waveformBuffer; }
    size_t getWaveformWritePos() const { return waveWriteIdx; }

private:
    std::atomic<float> rmsL{ 0.0f };
    std::atomic<float> rmsR{ 0.0f };
    std::vector<float> waveformBuffer;
    size_t waveWriteIdx = 0;
};

// GravRedshiftOscNode (grav.osc~)
class GravRedshiftOscNode : public RelativisticNode
{
public:
    GravRedshiftOscNode(int id, double mass = 1.0, double radius = 2.0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    double baseFreq = 440.0;
    double massM = 1.0;
    double radiusR = 2.0;
    double phase = 0.0;
};

// LorentzWarpFilterNode (lorentz~)
class LorentzWarpFilterNode : public RelativisticNode
{
public:
    LorentzWarpFilterNode(int id, double cutoff = 1200.0, double relVelocity = 0.5);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    double cutoffFreq = 1200.0;
    double velocityV = 0.5; // u/c ratio [0, 0.99]
    double s1 = 0.0;
    double s2 = 0.0;
};

// TachyonGranularNode (tachyon.grain~)
class TachyonGranularNode : public RelativisticNode
{
public:
    TachyonGranularNode(int id, double grainDurationMs = 50.0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;

private:
    double grainDurMs = 50.0;
    std::vector<float> sampleBuf;
    size_t writeHead = 0;
    double grainPhase = 0.0;
};

// MeterNode (meter~ / vu~) supporting Peak, RMS, and LUFS modes
class MeterNode : public RelativisticNode
{
public:
    enum class MeterMode { Peak, RMS, LUFS };

    MeterNode(int id, MeterMode mode = MeterMode::Peak);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    MeterMode getMeterMode() const { return meterMode; }
    void setMeterMode(MeterMode mode) { meterMode = mode; }
    std::string getMeterModeName() const;

    float getMeasuredLevelDb() const { return levelDb.load(); }
    float getPeakLevelDb() const { return peakDb.load(); }

private:
    MeterMode meterMode = MeterMode::Peak;
    std::atomic<float> levelDb{ -100.0f };
    std::atomic<float> peakDb{ -100.0f };

    // K-weighting filter state for LUFS calculation
    double lufsSampleRate = 48000.0;
    double preB0 = 1.0, preB1 = 0.0, preB2 = 0.0, preA1 = 0.0, preA2 = 0.0;
    double preZ1 = 0.0, preZ2 = 0.0;
    double rlhB0 = 1.0, rlhB1 = 0.0, rlhB2 = 0.0, rlhA1 = 0.0, rlhA2 = 0.0;
    double rlhZ1 = 0.0, rlhZ2 = 0.0;
    double lufsAccumulator = 0.0;
    int lufsSampleCount = 0;
};

// SpectrogramNode (spectrogram~ / spec~) for frequency vs time visualization up to Nyquist
class SpectrogramNode : public RelativisticNode
{
public:
    SpectrogramNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    static constexpr int fftOrder = 9; // 2^9 = 512
    static constexpr int fftSize = 512;
    static constexpr int numBins = fftSize / 2; // 256 frequency bins up to Nyquist
    static constexpr int historyLength = 128;   // 128 time frames

    const std::vector<float>& getSpectrogramGrid() const { return spectrogramGrid; }
    int getGridWriteIndex() const { return gridWritePos.load(); }
    double getNyquistFreq() const { return currentSampleRate * 0.5; }

private:
    juce::dsp::FFT fftEngine{ fftOrder };
    juce::dsp::WindowingFunction<float> window{ fftSize, juce::dsp::WindowingFunction<float>::hann };

    std::vector<float> fifoBuffer;
    size_t fifoWriteIdx = 0;

    std::vector<float> fftData; // 1024 floats (512 real + 512 imag)

    // Flattened grid of size (historyLength * numBins)
    std::vector<float> spectrogramGrid;
    std::atomic<int> gridWritePos{ 0 };

    bool isFrozen = false;
};

} // namespace TimeDilationDAW
