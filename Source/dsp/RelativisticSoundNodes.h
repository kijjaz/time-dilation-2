#pragma once

#include "RelativisticNodeGraph.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>

namespace TimeDilationDAW
{

struct TableInfo
{
    std::string name;
    std::string filePath;
    int numChannels = 1;
    double sampleRate = 44100.0;
    size_t numSamples = 0;
    double durationSec = 0.0;
    std::vector<float> thumbnailPeaks; // Pre-calculated peak amplitudes for UI thumbnail rendering
    std::vector<std::vector<float>> channelData; // Multi-channel sample data
};

// Global shared table memory pool for named tables & audio sample assets
class TableManager
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void onTablePoolChanged() = 0;
    };

    static TableManager& getInstance();
    void createTable(const std::string& name, size_t sizeInSamples);
    std::vector<float>& getTable(const std::string& name);
    bool hasTable(const std::string& name) const;

    bool loadSample(const std::string& name, const juce::File& file);
    void registerBuffer(const std::string& name, const juce::AudioBuffer<float>& buffer, double sampleRate, const std::string& sourcePath = "");
    const TableInfo* getTableInfo(const std::string& name) const;
    std::vector<std::string> getAllTableNames() const;
    void removeTable(const std::string& name);
    void renameTable(const std::string& oldName, const std::string& newName);
    void clearAllTables();

    // Batch Directory File Bundling
    void saveAllTablesToDirectory(const juce::File& audioDir);
    void loadTablesFromDirectory(const juce::File& audioDir);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    TableManager();
    mutable std::mutex mutex;
    std::unordered_map<std::string, std::vector<float>> tables;
    std::unordered_map<std::string, TableInfo> tableMetadata;
    std::vector<Listener*> listeners;
    void notifyListeners();
};

// High-Performance Global Sine Wavetable with 4-point Hermite interpolation (32-bit float accuracy)
class GlobalSineTable
{
public:
    static constexpr int TABLE_SIZE = 8192;
    static constexpr int TABLE_MASK = TABLE_SIZE - 1;

    static const GlobalSineTable& getInstance();
    float lookup(double phase) const noexcept;

private:
    GlobalSineTable();
    std::vector<float> table;
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

    float getSampleAtPhase(double p, double dt = 0.0) const;
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

// tabread4~ node: 4-point Hermite cubic-interpolated wavetable synthesizer oscillator & continuous table reader
class TabRead4TildeNode : public RelativisticNode
{
public:
    TabRead4TildeNode(int id, const std::string& tableName = "array1");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setTableName(const std::string& name) { tableName = name; setLabel("tabread4~ " + name); }
    const std::string& getTableName() const { return tableName; }

private:
    std::string tableName;
    double currentPhase = 0.0;
};

// tabplay~ node: Relativistic one-shot drum/sample player
class TabPlayTildeNode : public RelativisticNode
{
public:
    TabPlayTildeNode(int id, const std::string& tableName = "array1");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void startPlayback();
    void stopPlayback();
    void setTableName(const std::string& name) { tableName = name; setLabel("tabplay~ " + name); }
    const std::string& getTableName() const { return tableName; }
    bool isPlaying() const { return playingState.load(); }

private:
    std::string tableName;
    std::atomic<bool> playingState{ false };
    double playheadPosition = 0.0;
    double playbackSpeedFactor = 1.0;
    double pitchSemitones = 0.0;
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

// bang node (bng / bang)
class BangNode : public RelativisticNode
{
public:
    BangNode(int id);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void triggerBang();
    bool isFlashing() const;

private:
    std::atomic<double> flashTimer{ 0.0 };
};

// toggle node (tgl / toggle)
class ToggleNode : public RelativisticNode
{
public:
    ToggleNode(int id, bool initialState = false);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void toggleState();
    void setState(bool newState);
    bool getState() const { return state.load(); }

private:
    std::atomic<bool> state{ false };
};

// number node (num / number)
class NumberNode : public RelativisticNode
{
public:
    NumberNode(int id, double val = 0.0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void setValue(double newVal);
    double getValue() const { return value.load(); }

private:
    std::atomic<double> value{ 0.0 };
};

// symbol node (sym / symbol)
class SymbolNode : public RelativisticNode
{
public:
    SymbolNode(int id, const std::string& symText = "symbol");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void setSymbolText(const std::string& text);
    std::string getSymbolText() const;

private:
    std::string symbolText = "symbol";
};

// radio node (hradio / vradio / radio)
class RadioNode : public RelativisticNode
{
public:
    RadioNode(int id, int numOpts = 4, int initialIdx = 0);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void selectOption(int index);
    int getSelectedIndex() const { return selectedIdx.load(); }
    int getNumOptions() const { return numOptions; }

private:
    int numOptions = 4;
    std::atomic<int> selectedIdx{ 0 };
};

// display node (disp / display) - Visual canvas readout
class DisplayNode : public RelativisticNode
{
public:
    DisplayNode(int id, const std::string& sym = "disp", const std::string& tag = "");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    std::string getDisplayText() const;
    void setCustomTag(const std::string& tag) { customTag = tag; }
    const std::string& getCustomTag() const { return customTag; }

private:
    std::string customTag;
    std::string displayText = "---";
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
    float getPeakL() const { return peakL.load(); }
    float getPeakR() const { return peakR.load(); }
    bool isClippingL() const { return clipL.load(); }
    bool isClippingR() const { return clipR.load(); }
    const std::vector<float>& getWaveformBuffer() const { return waveformBuffer; }
    size_t getWaveformWritePos() const { return waveWriteIdx; }

private:
    std::atomic<float> rmsL{ 0.0f };
    std::atomic<float> rmsR{ 0.0f };
    std::atomic<float> peakL{ 0.0f };
    std::atomic<float> peakR{ 0.0f };
    std::atomic<bool> clipL{ false };
    std::atomic<bool> clipR{ false };
    int clipHoldCountL = 0;
    int clipHoldCountR = 0;
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

// pack~ / bundle~ / join~ node (Combines N mono audio inputs into 1 multichannel audio outlet)
class PackNode : public RelativisticNode
{
public:
    PackNode(int id, int numChannels = 2);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    int getNumChannels() const { return channelCount; }

private:
    int channelCount = 2;
};

// unpack~ / unbundle~ / split~ node (Splits 1 multichannel audio input into N mono audio outlets)
class UnpackNode : public RelativisticNode
{
public:
    UnpackNode(int id, int numChannels = 2);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    int getNumChannels() const { return channelCount; }

private:
    int channelCount = 2;
};

// reverb~ / freeverb~ node (Stereo algorithmic algorithmic reverberator)
class ReverbNode : public RelativisticNode
{
public:
    ReverbNode(int id, float roomSize = 0.7f, float damping = 0.4f, float wetLevel = 0.35f);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    juce::dsp::Reverb reverbEngine;
    juce::dsp::Reverb::Parameters reverbParams;
};

// noise~ node (White / Pink Noise Generator)
class NoiseNode : public RelativisticNode
{
public:
    NoiseNode(int id, const std::string& mode = "white");
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

private:
    juce::Random random;
    float pinkB0 = 0.0f, pinkB1 = 0.0f, pinkB2 = 0.0f;
    std::string noiseMode = "white";
};

// kick~ / drum.kick~ node (Analog Pitch-Sweep Sub-Bass Kick Drum)
class KickNode : public RelativisticNode
{
public:
    KickNode(int id, double basePitch = 50.0, double decaySec = 0.35);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void trigger();

private:
    double baseFreq = 50.0;
    double decayTime = 0.35;
    double envPhase = 1.0;
    double oscPhase = 0.0;
    float lastTrigVal = 0.0f;
    std::atomic<bool> isTriggered{ false };
};

// snare~ / drum.snare~ node (Analog Dual-Resonator & Filtered Noise Snare Drum)
class SnareNode : public RelativisticNode
{
public:
    SnareNode(int id, double toneFreq = 185.0, double snappy = 0.65, double decaySec = 0.28);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void trigger();

private:
    double toneFrequency = 185.0;
    double snappyAmount = 0.65;
    double decayTime = 0.28;
    double envPhase = 1.0;
    double bodyPhase1 = 0.0;
    double bodyPhase2 = 0.0;
    float lastTrigVal = 0.0f;
    juce::Random random;
    std::atomic<bool> isTriggered{ false };
};

// hihat~ / drum.hat~ node (Metallic Multi-Pulse Closed/Open Hi-Hat)
class HiHatNode : public RelativisticNode
{
public:
    HiHatNode(int id, double decaySec = 0.08);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;
    void trigger(double decay = 0.08);

private:
    double decayTime = 0.08;
    double envPhase = 1.0;
    float lastTrigVal = 0.0f;
    double phases[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    static constexpr double freqs[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
    std::atomic<bool> isTriggered{ false };
};

} // namespace TimeDilationDAW
