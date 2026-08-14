#include "PrintNode.h"
#include <cmath>
#include <sstream>

namespace TimeDilationDAW
{

PrintNode::PrintNode(int id, const std::string& prefix, bool audioMode)
    : RelativisticNode(id, audioMode ? "print~" : "print", (audioMode ? "print~ " : "print ") + prefix),
      prefixTag(prefix.empty() ? (audioMode ? "print~" : "print") : prefix),
      isAudioProbe(audioMode)
{
    addInlet("msgIn", PortDataType::Message);   // Inlet 0: Message Input (Gold)
    addInlet("in~", PortDataType::Audio);       // Inlet 1: Audio Signal Input (Cyan)

    addOutlet("msgOut", PortDataType::Message); // Outlet 0: Message Passthrough (Gold)
    addOutlet("out~", PortDataType::Audio);     // Outlet 1: Audio Signal Passthrough (Cyan)
}

void PrintNode::prepare(double sampleRate, int samplesPerBlock)
{
    RelativisticNode::prepare(sampleRate, samplesPerBlock);
    probeIntervalSamples = static_cast<int>(std::max(1000.0, sampleRate * 0.15)); // ~150ms report rate
    samplesSinceLastProbe = 0;
    currentPeak = 0.0f;
    currentRms = 0.0f;
    currentEnv = 0.0f;
}

void PrintNode::process(int numSamples)
{
    const auto& inBuf = getInletBuffer(1);
    auto& outBuf = getOutletBuffer(1);

    if (outBuf.getNumChannels() < 1 || outBuf.getNumSamples() < numSamples)
    {
        outBuf.setSize(1, numSamples, false, false, true);
    }

    float* out = outBuf.getWritePointer(0);

    if (inBuf.getNumChannels() > 0 && numSamples > 0)
    {
        const float* in = inBuf.getReadPointer(0);
        float peak = 0.0f;
        float sumSq = 0.0f;

        for (int s = 0; s < numSamples; ++s)
        {
            float val = in[s];
            out[s] = val; // Direct transparent passthrough

            float absVal = std::abs(val);
            if (absVal > peak) peak = absVal;
            sumSq += val * val;
        }

        currentPeak = std::max(currentPeak * 0.95f, peak);
        currentRms = std::sqrt(sumSq / static_cast<float>(numSamples));
        currentEnv = currentPeak;

        samplesSinceLastProbe += numSamples;
        if (isAudioProbe && samplesSinceLastProbe >= probeIntervalSamples)
        {
            samplesSinceLastProbe = 0;
            if (currentPeak > 0.0001f) // Only log when active or on periodic pulse
            {
                triggerProbe();
            }
        }
    }
    else
    {
        outBuf.clear();
    }
}

void PrintNode::triggerProbe()
{
    float peakDb = (currentPeak > 1e-5f) ? juce::Decibels::gainToDecibels(currentPeak, -100.0f) : -100.0f;
    float rmsDb = (currentRms > 1e-5f) ? juce::Decibels::gainToDecibels(currentRms, -100.0f) : -100.0f;
    bool active = (currentPeak > 0.005f);

    ConsoleLogger::getInstance().logProbe(prefixTag, peakDb, rmsDb, currentEnv, active);
}

void PrintNode::receiveMessage(const std::string& message)
{
    RelativisticNode::receiveMessage(message);

    std::string s = message;
    if (s == "probe" || s == "bang" || s == "stat" || s == "stats")
    {
        triggerProbe();
    }
    else if (s.rfind("prefix ", 0) == 0 || s.rfind("set ", 0) == 0)
    {
        prefixTag = s.substr(s.find(' ') + 1);
        setLabel((isAudioProbe ? "print~ " : "print ") + prefixTag);
    }
    else
    {
        // Standard message print
        ConsoleLogger::getInstance().log(s, prefixTag, LogLevel::Message);
    }

    // Forward message to msgOut
    emitMessageOnMsgOut(s);
}

} // namespace TimeDilationDAW
