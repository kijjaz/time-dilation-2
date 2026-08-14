#include "PdSampleNodes.h"
#include "../utils/ConsoleLogger.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace TimeDilationDAW
{

// ============================================================================
// SoundfilerNode ([soundfiler])
// ============================================================================

SoundfilerNode::SoundfilerNode(int id)
    : RelativisticNode(id, "soundfiler", "soundfiler")
{
    formatManager.registerBasicFormats();
    addInlet("msgIn", PortDataType::Message);
    addOutlet("msgOut", PortDataType::Message);
}

void SoundfilerNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void SoundfilerNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void SoundfilerNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msg, " ", "\"");

    if (tokens.isEmpty()) return;

    if (tokens[0] == "read")
    {
        bool resize = false;
        juce::String filePath;
        std::string tableName = "array1";

        int idx = 1;
        while (idx < tokens.size() && tokens[idx].startsWith("-"))
        {
            if (tokens[idx] == "-resize") resize = true;
            ++idx;
        }

        if (idx < tokens.size())
        {
            filePath = tokens[idx++];
        }
        if (idx < tokens.size())
        {
            tableName = tokens[idx++].toStdString();
        }

        juce::File audioFile(filePath);
        if (!audioFile.existsAsFile())
        {
            // Try relative to current directory or user home
            audioFile = juce::File::getCurrentWorkingDirectory().getChildFile(filePath);
        }

        bool success = readFile(audioFile, tableName, resize);
        if (!success)
        {
            ConsoleLogger::getInstance().log("soundfiler read error: could not open " + filePath.toStdString(), "soundfiler", LogLevel::Error);
            emitMessageOnMsgOut("0");
        }
    }
    else if (tokens[0] == "write")
    {
        juce::String filePath;
        std::string tableName = "array1";

        if (tokens.size() > 1) filePath = tokens[1];
        if (tokens.size() > 2) tableName = tokens[2].toStdString();

        juce::File targetFile(filePath);
        bool success = writeFile(targetFile, tableName);
        if (!success)
        {
            ConsoleLogger::getInstance().log("soundfiler write error: could not save to " + filePath.toStdString(), "soundfiler", LogLevel::Error);
        }
    }
}

bool SoundfilerNode::readFile(const juce::File& file, const std::string& tableName, bool resizeTable)
{
    if (!file.existsAsFile()) return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;

    size_t numSamples = static_cast<size_t>(reader->lengthInSamples);
    if (numSamples == 0) return false;

    if (resizeTable || !TableManager::getInstance().hasTable(tableName))
    {
        TableManager::getInstance().createTable(tableName, numSamples);
    }

    auto& table = TableManager::getInstance().getTable(tableName);
    size_t samplesToRead = std::min(numSamples, table.size());

    juce::AudioBuffer<float> tempBuf(static_cast<int>(reader->numChannels), static_cast<int>(samplesToRead));
    reader->read(&tempBuf, 0, static_cast<int>(samplesToRead), 0, true, true);

    // If stereo, mixdown to mono for single table
    if (tempBuf.getNumChannels() == 1)
    {
        const float* readPtr = tempBuf.getReadPointer(0);
        for (size_t i = 0; i < samplesToRead; ++i) table[i] = readPtr[i];
    }
    else
    {
        const float* lPtr = tempBuf.getReadPointer(0);
        const float* rPtr = tempBuf.getReadPointer(1);
        for (size_t i = 0; i < samplesToRead; ++i) table[i] = 0.5f * (lPtr[i] + rPtr[i]);
    }

    ConsoleLogger::getInstance().log("soundfiler read " + std::to_string(samplesToRead) + " samples into table '" + tableName + "'", "soundfiler", LogLevel::System);
    emitMessageOnMsgOut(std::to_string(samplesToRead));
    return true;
}

bool SoundfilerNode::writeFile(const juce::File& file, const std::string& tableName)
{
    if (!TableManager::getInstance().hasTable(tableName)) return false;

    const auto& table = TableManager::getInstance().getTable(tableName);
    if (table.empty()) return false;

    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> outStream(file.createOutputStream());
    if (!outStream) return false;

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outStream.get(), 44100.0, 1, 16, {}, 0));

    if (!writer) return false;
    outStream.release(); // Writer took ownership

    juce::AudioBuffer<float> buf(1, static_cast<int>(table.size()));
    float* writePtr = buf.getWritePointer(0);
    for (size_t i = 0; i < table.size(); ++i) writePtr[i] = table[i];

    writer->writeFromAudioSampleBuffer(buf, 0, static_cast<int>(table.size()));
    ConsoleLogger::getInstance().log("soundfiler wrote " + std::to_string(table.size()) + " samples to " + file.getFullPathName().toStdString(), "soundfiler", LogLevel::System);
    return true;
}


// ============================================================================
// ReadSFTildeNode ([readsf~])
// ============================================================================

ReadSFTildeNode::ReadSFTildeNode(int id, int numChannels)
    : RelativisticNode(id, "readsf~", "readsf~ " + std::to_string(numChannels)), channelCount(std::clamp(numChannels, 1, 2))
{
    formatManager.registerBasicFormats();

    addInlet("timeIn", PortDataType::Time);
    addOutlet("timeOut", PortDataType::Time);
    addOutlet("out1~", PortDataType::Audio);
    if (channelCount >= 2)
    {
        addOutlet("out2~", PortDataType::Audio);
    }
}

void ReadSFTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

bool ReadSFTildeNode::openFile(const juce::File& file)
{
    if (!file.existsAsFile()) return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;

    fileSampleRate = reader->sampleRate;
    int numSamples = static_cast<int>(reader->lengthInSamples);
    if (numSamples <= 0) return false;

    fileData.setSize(std::max(2, static_cast<int>(reader->numChannels)), numSamples);
    reader->read(&fileData, 0, numSamples, 0, true, true);

    // If source file is mono, copy Ch 0 to Ch 1 for stereo playback
    if (reader->numChannels == 1 && fileData.getNumChannels() >= 2)
    {
        fileData.copyFrom(1, 0, fileData, 0, 0, numSamples);
    }

    currentFile = file;
    playheadPosition = 0.0;
    ConsoleLogger::getInstance().log("readsf~ opened: " + file.getFileName().toStdString() + " (" + std::to_string(numSamples) + " samples)", "readsf~", LogLevel::System);
    return true;
}

void ReadSFTildeNode::startPlayback()
{
    if (fileData.getNumSamples() > 0)
    {
        playingState = true;
    }
}

void ReadSFTildeNode::stopPlayback()
{
    playingState = false;
}

void ReadSFTildeNode::seekSeconds(double seconds)
{
    if (fileSampleRate > 1.0)
    {
        playheadPosition = std::clamp(seconds * fileSampleRate, 0.0, static_cast<double>(fileData.getNumSamples() - 1));
    }
}

double ReadSFTildeNode::getCurrentPositionSec() const
{
    return (fileSampleRate > 1.0) ? (playheadPosition / fileSampleRate) : 0.0;
}

double ReadSFTildeNode::getDurationSec() const
{
    return (fileSampleRate > 1.0 && fileData.getNumSamples() > 0) ? (static_cast<double>(fileData.getNumSamples()) / fileSampleRate) : 0.0;
}

void ReadSFTildeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    juce::String msg = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msg, " ", "\"");

    if (tokens.isEmpty()) return;

    if (tokens[0] == "open" && tokens.size() > 1)
    {
        juce::File f(tokens[1]);
        if (!f.existsAsFile())
        {
            f = juce::File::getCurrentWorkingDirectory().getChildFile(tokens[1]);
        }
        openFile(f);
    }
    else if (tokens[0] == "start" || tokens[0] == "1" || tokens[0] == "bang")
    {
        startPlayback();
    }
    else if (tokens[0] == "stop" || tokens[0] == "0")
    {
        stopPlayback();
    }
    else if (tokens[0] == "seek" && tokens.size() > 1)
    {
        seekSeconds(tokens[1].getDoubleValue());
    }
    else if (tokens[0] == "speed" && tokens.size() > 1)
    {
        setSpeed(tokens[1].getDoubleValue());
    }
    else if (tokens[0] == "loop" && tokens.size() > 1)
    {
        setLooping(tokens[1].getIntValue() != 0);
    }
}

void ReadSFTildeNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    auto& outLBuf = getAudioOutlet("out1~");
    if (outLBuf.getNumChannels() < 1 || outLBuf.getNumSamples() < numSamples)
    {
        outLBuf.setSize(1, numSamples, false, false, true);
    }
    outLBuf.clear();

    juce::AudioBuffer<float>* outRBuf = nullptr;
    if (channelCount >= 2)
    {
        outRBuf = &getAudioOutlet("out2~");
        if (outRBuf->getNumChannels() < 1 || outRBuf->getNumSamples() < numSamples)
        {
            outRBuf->setSize(1, numSamples, false, false, true);
        }
        outRBuf->clear();
    }

    const auto& timeFrame = getTimeInlet("timeIn");
    getTimeOutlet("timeOut") = timeFrame;

    if (!playingState || fileData.getNumSamples() == 0)
    {
        return;
    }

    float* outL = outLBuf.getWritePointer(0);
    float* outR = (outRBuf != nullptr) ? outRBuf->getWritePointer(0) : nullptr;

    const float* fileL = fileData.getReadPointer(0);
    const float* fileR = (fileData.getNumChannels() >= 2) ? fileData.getReadPointer(1) : fileL;
    int totalSamples = fileData.getNumSamples();

    double sRate = (currentSampleRate > 1.0) ? currentSampleRate : 96000.0;
    double sampleRateRatio = fileSampleRate / sRate;

    const bool hasAudioRateGamma = (timeFrame.sampleGamma.size() >= static_cast<size_t>(numSamples));

    for (int s = 0; s < numSamples; ++s)
    {
        double currentGamma = hasAudioRateGamma ? static_cast<double>(timeFrame.sampleGamma[static_cast<size_t>(s)]) : timeFrame.masterGamma;
        double effectiveSpeed = playbackSpeed * currentGamma * sampleRateRatio;

        int idx0 = static_cast<int>(std::floor(playheadPosition));
        double frac = playheadPosition - static_cast<double>(idx0);

        if (idx0 >= 0 && idx0 < totalSamples)
        {
            int idx1 = std::min(totalSamples - 1, idx0 + 1);

            // Linear interpolation
            float sampL = static_cast<float>((1.0 - frac) * fileL[idx0] + frac * fileL[idx1]);
            float sampR = static_cast<float>((1.0 - frac) * fileR[idx0] + frac * fileR[idx1]);

            outL[s] = sampL;
            if (outR) outR[s] = sampR;
        }
        else
        {
            outL[s] = 0.0f;
            if (outR) outR[s] = 0.0f;
        }

        playheadPosition += effectiveSpeed;

        if (playheadPosition >= static_cast<double>(totalSamples))
        {
            if (isLooping)
            {
                playheadPosition = std::fmod(playheadPosition, static_cast<double>(totalSamples));
            }
            else
            {
                playingState = false;
                emitMessageOnMsgOut("bang"); // EOF bang
                break;
            }
        }
        else if (playheadPosition < 0.0)
        {
            if (isLooping)
            {
                playheadPosition = static_cast<double>(totalSamples) + std::fmod(playheadPosition, static_cast<double>(totalSamples));
            }
            else
            {
                playingState = false;
                emitMessageOnMsgOut("bang");
                break;
            }
        }
    }
}

} // namespace TimeDilationDAW
