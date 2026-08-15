#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "RelativisticNodeGraph.h"

namespace TimeDilationDAW
{

enum class AudioInputType
{
    ExternalMono,    // Channel 1, 2, ...
    ExternalStereo,  // Channels (1, 2), etc.
    InternalMaster,  // Master stereo output bus
    InternalNodeTap  // Specific node ID + outlet index
};

struct AudioInputSource
{
    AudioInputType type = AudioInputType::ExternalStereo;
    int primaryChannel = 1;   // 1-indexed for hardware (1 = In 1, 2 = In 2)
    int secondaryChannel = 2; // 2 = In 2
    int tapNodeId = -1;       // For InternalNodeTap
    int tapOutletIndex = 0;   // Outlet index on tapNodeId

    std::string getDisplayName() const
    {
        switch (type)
        {
            case AudioInputType::ExternalMono:
                return "In " + std::to_string(primaryChannel) + " (Mono)";
            case AudioInputType::ExternalStereo:
                return "In " + std::to_string(primaryChannel) + "+" + std::to_string(secondaryChannel) + " (Stereo)";
            case AudioInputType::InternalMaster:
                return "Master Mix (Internal)";
            case AudioInputType::InternalNodeTap:
                return "Tap: Node " + std::to_string(tapNodeId);
        }
        return "Stereo In";
    }
};

class AudioInputRouter
{
public:
    static AudioInputRouter& getInstance();

    void fetchAudioBlock(const AudioInputSource& source,
                         RelativisticNodeGraph& graph,
                         juce::AudioBuffer<float>& destinationBuffer,
                         int numSamples);

    static void setGlobalInputBuffer(const juce::AudioBuffer<float>& inBuf);
    static const juce::AudioBuffer<float>& getGlobalInputBuffer();

    static void setMasterOutputBuffer(const juce::AudioBuffer<float>& outBuf);
    static const juce::AudioBuffer<float>& getMasterOutputBuffer();

private:
    AudioInputRouter() = default;

    static juce::AudioBuffer<float> globalInputBuffer;
    static juce::AudioBuffer<float> masterOutputBuffer;
    static juce::SpinLock bufferLock;
};

} // namespace TimeDilationDAW
