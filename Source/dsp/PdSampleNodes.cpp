#include "PdSampleNodes.h"
#include "../utils/ConsoleLogger.h"
#include "../utils/ProjectManager.h"
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

        juce::File audioFile = ProjectManager::getInstance().resolveAudioFile(filePath);

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

        juce::File targetFile = ProjectManager::getInstance().resolveAudioFile(filePath);
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
        juce::File f = ProjectManager::getInstance().resolveAudioFile(tokens[1]);
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


// ============================================================================
// AdcNode Implementation ([adc~], [in~])
// ============================================================================

juce::AudioBuffer<float> AdcNode::globalInputBuffer;
juce::SpinLock AdcNode::globalInputLock;

AdcNode::AdcNode(int id, const std::vector<int>& channelList)
    : RelativisticNode(id, "adc~", "adc~")
{
    setChannels(channelList.empty() ? std::vector<int>{ 1, 2 } : channelList);
}

void AdcNode::setChannels(const std::vector<int>& channelList)
{
    targetChannels = channelList;
    outlets.clear();
    outletBuffers.clear();
    outletTimeFrames.clear();

    std::string lbl = "adc~";
    for (size_t i = 0; i < targetChannels.size(); ++i)
    {
        lbl += " " + std::to_string(targetChannels[i]);
        addOutlet("out" + std::to_string(i + 1) + "~", PortDataType::Audio);
    }
    setLabel(lbl);
}

void AdcNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void AdcNode::setGlobalInputBuffer(const juce::AudioBuffer<float>& inBuf)
{
    const juce::SpinLock::ScopedLockType sl(globalInputLock);
    globalInputBuffer.makeCopyOf(inBuf, true);
}

const juce::AudioBuffer<float>& AdcNode::getGlobalInputBuffer()
{
    return globalInputBuffer;
}

void AdcNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    juce::String msg = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msg, " ", "");

    if (tokens.size() > 1 && (tokens[0] == "set" || tokens[0] == "ch"))
    {
        std::vector<int> chs;
        for (int i = 1; i < tokens.size(); ++i)
        {
            int ch = tokens[i].getIntValue();
            if (ch > 0) chs.push_back(ch);
        }
        if (!chs.empty()) setChannels(chs);
    }
}

void AdcNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    const juce::SpinLock::ScopedLockType sl(globalInputLock);
    float sumSquares = 0.0f;
    int totalSampleCount = 0;

    for (size_t i = 0; i < targetChannels.size(); ++i)
    {
        int chIdx = targetChannels[i] - 1; // 1-indexed to 0-indexed
        auto& outBuf = getOutletBuffer(static_cast<int>(i));

        if (outBuf.getNumSamples() < numSamples)
        {
            outBuf.setSize(1, numSamples, false, false, true);
        }

        float* outPtr = outBuf.getWritePointer(0);

        if (chIdx >= 0 && chIdx < globalInputBuffer.getNumChannels() && globalInputBuffer.getNumSamples() >= numSamples)
        {
            const float* inPtr = globalInputBuffer.getReadPointer(chIdx);
            for (int s = 0; s < numSamples; ++s)
            {
                float val = inPtr[s];
                outPtr[s] = val;
                sumSquares += val * val;
                totalSampleCount++;
            }
        }
        else
        {
            outBuf.clear();
        }
    }

    if (totalSampleCount > 0)
    {
        rmsLevel.store(std::sqrt(sumSquares / static_cast<float>(totalSampleCount)), std::memory_order_relaxed);
    }
}


// ============================================================================
// TabWriteTildeNode Implementation ([tabwrite~])
// ============================================================================

TabWriteTildeNode::TabWriteTildeNode(int id, const std::string& tableName)
    : RelativisticNode(id, "tabwrite~", "tabwrite~ " + (tableName.empty() ? "rec_buf" : tableName))
    , targetTable(tableName.empty() ? "rec_buf" : tableName)
{
    // Inlet 0: msgIn (from base)
    // Inlet 1: in1~ (Audio input to record)
    addInlet("in1~", PortDataType::Audio);
    // Inlet 2: timeIn (Time stream)
    addInlet("timeIn", PortDataType::Time);

    // Outlet 0: done (Message bang upon finish)
    if (!outlets.empty()) outlets[0].name = "done";

    recordBuffer.reserve(44100 * 10);
}

void TabWriteTildeNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TabWriteTildeNode::setTableName(const std::string& name)
{
    targetTable = name.empty() ? "rec_buf" : name;
    setLabel("tabwrite~ " + targetTable);
}

void TabWriteTildeNode::startRecording(int maxSamplesToRecord)
{
    if (maxSamplesToRecord > 0)
    {
        maxSamples = static_cast<size_t>(maxSamplesToRecord);
    }
    else
    {
        maxSamples = static_cast<size_t>((currentSampleRate > 1.0 ? currentSampleRate : 44100.0) * 10.0);
    }

    recordBuffer.clear();
    recordBuffer.reserve(maxSamples);
    writePos = 0;
    recordingActive = true;
}

void TabWriteTildeNode::stopRecording()
{
    if (!recordingActive && writePos == 0) return;
    recordingActive = false;

    if (!recordBuffer.empty())
    {
        juce::AudioBuffer<float> finalBuf(1, static_cast<int>(recordBuffer.size()));
        finalBuf.copyFrom(0, 0, recordBuffer.data(), static_cast<int>(recordBuffer.size()));
        TableManager::getInstance().registerBuffer(targetTable, finalBuf, currentSampleRate > 1.0 ? currentSampleRate : 44100.0);
    }

    emitMessageOnOutlet(0, "bang");
}

void TabWriteTildeNode::clearBuffer()
{
    recordBuffer.clear();
    writePos = 0;
    recordingActive = false;
}

void TabWriteTildeNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    juce::String msg = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msg, " ", "");

    if (tokens.isEmpty()) return;

    if (tokens[0] == "start" || tokens[0] == "bang" || tokens[0] == "1")
    {
        int maxS = (tokens.size() > 1) ? tokens[1].getIntValue() : -1;
        startRecording(maxS);
    }
    else if (tokens[0] == "stop" || tokens[0] == "0")
    {
        stopRecording();
    }
    else if (tokens[0] == "clear")
    {
        clearBuffer();
    }
    else if (tokens[0] == "set" && tokens.size() > 1)
    {
        setTableName(tokens[1].toStdString());
    }
    else if (tokens[0] == "resize" && tokens.size() > 1)
    {
        maxSamples = static_cast<size_t>(tokens[1].getIntValue());
    }
    else
    {
        // Treat as table name change if not keyword
        setTableName(tokens[0].toStdString());
    }
}

void TabWriteTildeNode::process(int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    if (!recordingActive) return;

    const auto& inBuf = getAudioInlet("in1~");
    if (inBuf.getNumSamples() < numSamples) return;

    const float* inPtr = inBuf.getReadPointer(0);

    for (int s = 0; s < numSamples; ++s)
    {
        if (writePos < maxSamples)
        {
            recordBuffer.push_back(inPtr[s]);
            writePos++;
        }
        else
        {
            stopRecording();
            break;
        }
    }
}


// ============================================================================
// TabWriteNode Implementation ([tabwrite])
// ============================================================================

TabWriteNode::TabWriteNode(int id, const std::string& tableName)
    : RelativisticNode(id, "tabwrite", "tabwrite " + (tableName.empty() ? "table1" : tableName))
    , targetTable(tableName.empty() ? "table1" : tableName)
{
}

void TabWriteNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
}

void TabWriteNode::process(int numSamples)
{
    juce::ignoreUnused(numSamples);
}

void TabWriteNode::setTableName(const std::string& name)
{
    targetTable = name.empty() ? "table1" : name;
    setLabel("tabwrite " + targetTable);
}

void TabWriteNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);
    juce::String msg = juce::String(message).trim();
    juce::StringArray tokens;
    tokens.addTokens(msg, " ", "");

    if (tokens.size() >= 2)
    {
        if (tokens[0] == "set")
        {
            setTableName(tokens[1].toStdString());
        }
        else
        {
            // Protocol: "<value> <index>"
            float val = tokens[0].getFloatValue();
            int idx = tokens[1].getIntValue();

            if (TableManager::getInstance().hasTable(targetTable))
            {
                auto& tbl = TableManager::getInstance().getTable(targetTable);
                if (idx >= 0 && static_cast<size_t>(idx) < tbl.size())
                {
                    tbl[static_cast<size_t>(idx)] = val;
                }
            }
        }
    }
}

} // namespace TimeDilationDAW
