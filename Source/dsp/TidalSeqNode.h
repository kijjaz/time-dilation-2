#pragma once

#include "RelativisticNodeGraph.h"
#include "TidalPatternEngine.h"
#include <string>
#include <vector>
#include <memory>

namespace TimeDilationDAW
{

// ==============================================================================
// TidalSeqNode ([seq.tidal], [tidal], [pattern])
// Relativistic TidalCycles Mini-Notation Sequencer
// Clocked by proper-time cycle integration phi(t) = (1/cycleDuration) * integral(gamma(t) dt)
// ==============================================================================
class TidalSeqNode : public RelativisticNode
{
public:
    TidalSeqNode(int id, const std::string& patternString = "[60 [62 64] 67 [69 71 72]]", double cycleDurationSec = 2.0);

    void prepare(double sampleRate, int samplesPerBlock) override;
    void process(int numSamples) override;
    void receiveMessage(const std::string& message) override;

    void setPattern(const std::string& patternString);
    const std::string& getPatternString() const { return currentPatternStr; }

    void setCycleDuration(double durSec) { cycleDurationSec = std::max(0.05, durSec); }
    double getCycleDuration() const { return cycleDurationSec; }

    double getCyclePhase() const { return cyclePhase; }
    int getCycleCount() const { return cycleCount; }
    int getNumVoices() const { return activeVoices; }
    const std::vector<TidalEvent>& getScheduledEvents() const { return scheduledEvents; }

private:
    void evaluateCurrentCycle();
    void rebuildOutlets(int numVoices);

    std::string currentPatternStr;
    std::shared_ptr<TidalPattern> compiledPattern;
    double cycleDurationSec = 2.0;

    double cyclePhase = 0.0;     // Normalized cycle phase in [0.0, 1.0)
    int cycleCount = 0;
    int activeVoices = 1;

    std::vector<TidalEvent> scheduledEvents;
    size_t nextEventIdx = 0;
};

} // namespace TimeDilationDAW
