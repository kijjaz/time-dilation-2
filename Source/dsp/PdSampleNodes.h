#pragma once

#include "RelativisticNodeGraph.h"
#include "RelativisticSoundNodes.h"
#include "AudioInputRouter.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <memory>
#include <string>
#include <vector>

namespace TimeDilationDAW
{

// ============================================================================
// SoundfilerNode ([soundfiler])
// Reads/writes audio files (WAV, AIFF, FLAC, OGG, MP3) to/from TableManager tables.
// Protocol: "read [-resize] <filepath> <tablename>" or "write <filepath> <tablename>"
// Outputs loaded sample count out message outlet.
// ============================================================================
class SoundfilerNode : public RelativisticNode
{
public:
    explicit SoundfilerNode(int id);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    bool readFile(const juce::File& file, const std::string& tableName, bool resizeTable);
    bool writeFile(const juce::File& file, const std::string& tableName);

private:
    juce::AudioFormatManager formatManager;
};


// ============================================================================
// ReadSFTildeNode ([readsf~])
// Multi-channel audio file streaming player with relativistic time scrubbing.
// Commands: "open <filepath>", "start", "stop", "seek <seconds>", "loop 0/1", "speed <factor>"
// Outlets: msgOut (EOF bang), timeOut (Time), outL~ (Audio Left), outR~ (Audio Right)
// ============================================================================
class ReadSFTildeNode : public RelativisticNode
{
public:
    explicit ReadSFTildeNode(int id, int numChannels = 2);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    bool openFile(const juce::File& file);
    void startPlayback();
    void stopPlayback();
    void seekSeconds(double seconds);
    void setLooping(bool loop) { isLooping = loop; }
    void setSpeed(double spd) { playbackSpeed = spd; }

    bool isPlaying() const { return playingState; }
    double getCurrentPositionSec() const;
    double getDurationSec() const;

private:
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> fileData;
    double fileSampleRate = 44100.0;
    double playheadPosition = 0.0; // In file samples
    double playbackSpeed = 1.0;
    bool playingState = false;
    bool isLooping = false;
    int channelCount = 2;
    juce::File currentFile;
};


// ============================================================================
// AdcNode ([adc~], [in~])
// Hardware / Live Audio Interface Input Capture Node.
// Protocol: "adc~ [ch1] [ch2] ..." (Default: 1 2 for Stereo Left/Right).
// Outlets: out1~ (Audio Left), out2~ (Audio Right).
// ============================================================================
class AdcNode : public RelativisticNode
{
public:
    explicit AdcNode(int id, const std::vector<int>& channelList = { 1, 2 });

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setChannels(const std::vector<int>& channelList);
    const std::vector<int>& getChannels() const { return targetChannels; }

    // Static buffer injection for live hardware callback and headless testing
    static void setGlobalInputBuffer(const juce::AudioBuffer<float>& inBuf);
    static const juce::AudioBuffer<float>& getGlobalInputBuffer();

private:
    std::vector<int> targetChannels;
    static juce::AudioBuffer<float> globalInputBuffer;
    static juce::SpinLock globalInputLock;
};


// ============================================================================
// TabWriteTildeNode ([tabwrite~])
// Relativistic Real-Time Audio Buffer Recorder into TableManager Tables.
// Inlets: msgIn (Message: "start", "stop", "clear", "bang", "resize <N>"), in1~ (Audio), timeIn (Time)
// Outlets: done (Message bang upon finish)
// ============================================================================
class TabWriteTildeNode : public RelativisticNode
{
public:
    explicit TabWriteTildeNode(int id, const std::string& tableName = "rec_buf");

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setTableName(const std::string& name);
    const std::string& getTableName() const { return targetTable; }

    void setInputSource(const AudioInputSource& src) { inputSource = src; }
    const AudioInputSource& getInputSource() const { return inputSource; }

    void startRecording(int maxSamplesToRecord = -1);
    void stopRecording();
    void clearBuffer();
    bool isRecording() const { return recordingActive; }

    size_t getRecordedSamples() const { return writePos; }

private:
    std::string targetTable;
    AudioInputSource inputSource;
    bool recordingActive = false;
    size_t writePos = 0;
    size_t maxSamples = 44100 * 10; // Default 10 seconds capacity
    std::vector<float> recordBuffer;
};


// ============================================================================
// TabWriteNode ([tabwrite])
// Message/Control-rate table value writer into TableManager tables.
// Protocol: "<value> <index>" or "set <index> <value>"
// ============================================================================
class TabWriteNode : public RelativisticNode
{
public:
    explicit TabWriteNode(int id, const std::string& tableName = "table1");

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setTableName(const std::string& name);
    const std::string& getTableName() const { return targetTable; }

private:
    std::string targetTable;
};

} // namespace TimeDilationDAW
