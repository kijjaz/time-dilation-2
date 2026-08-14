#pragma once

#include "RelativisticNodeGraph.h"
#include "RelativisticSoundNodes.h"
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

} // namespace TimeDilationDAW
