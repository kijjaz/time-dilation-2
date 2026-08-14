#pragma once

#include "RelativisticNodeGraph.h"
#include "../utils/ConsoleLogger.h"
#include <string>

namespace TimeDilationDAW
{

// [print] / [print~] - Message, Number, and Audio Signal/Envelope Console Logger
class PrintNode : public RelativisticNode
{
public:
    PrintNode(int id, const std::string& prefix = "print", bool audioMode = false);
    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setPrefix(const std::string& newPrefix) { prefixTag = newPrefix; }
    std::string getPrefix() const { return prefixTag; }

    void triggerProbe();

private:
    std::string prefixTag = "print";
    bool isAudioProbe = false;
    int samplesSinceLastProbe = 0;
    int probeIntervalSamples = 9600; // ~100ms at 96kHz (10 Hz periodic report)
    float currentPeak = 0.0f;
    float currentRms = 0.0f;
    float currentEnv = 0.0f;
};

} // namespace TimeDilationDAW
