#include "TidalSeqNode.h"
#include <sstream>

namespace TimeDilationDAW
{

TidalSeqNode::TidalSeqNode(int id, const std::string& patternString, double cycleDur)
    : RelativisticNode(id, "seq.tidal", "seq.tidal " + patternString)
    , cycleDurationSec(cycleDur)
{
    // Inlets:
    // 0: msgIn (set pattern / speed), 1: timeIn (TimeFrame)
    addInlet("timeIn", PortDataType::Time);

    // Outlets:
    // 0: noteOut (Msg), 1: freqOut~ (Audio), 2: gateOut (Msg bang), 3: ch2NoteOut (Msg), 4: audioTrig~ (Audio)
    addOutlet("noteOut", PortDataType::Message);
    addOutlet("freqOut", PortDataType::Audio);
    addOutlet("gateOut", PortDataType::Message);
    addOutlet("ch2NoteOut", PortDataType::Message);
    addOutlet("audioTrig", PortDataType::Audio);

    setPattern(patternString);
}

void TidalSeqNode::setPattern(const std::string& patternString)
{
    std::string cleanPat = patternString;
    // Strip leading "seq.tidal ", "tidal ", "pattern ", "pat " or "set " if present
    for (const auto& prefix : { "seq.tidal ", "tidal ", "pattern ", "pat ", "set " })
    {
        if (cleanPat.rfind(prefix, 0) == 0)
        {
            cleanPat = cleanPat.substr(std::string(prefix).length());
            size_t first = cleanPat.find_first_not_of(" \t");
            if (first != std::string::npos) cleanPat = cleanPat.substr(first);
            break;
        }
    }

    if (cleanPat.empty()) cleanPat = "[60 [62 64] 67 [69 71 72]]";

    currentPatternStr = cleanPat;
    setLabel("seq.tidal " + cleanPat);
    compiledPattern = TidalParser::parse(cleanPat);
    evaluateCurrentCycle();
}

void TidalSeqNode::evaluateCurrentCycle()
{
    scheduledEvents.clear();
    nextEventIdx = 0;
    if (compiledPattern)
    {
        compiledPattern->query(0.0, 1.0, cycleCount, scheduledEvents);
        // Sort events chronologically by startCycle
        std::sort(scheduledEvents.begin(), scheduledEvents.end(), [](const TidalEvent& a, const TidalEvent& b) {
            return a.startCycle < b.startCycle;
        });
    }
}

void TidalSeqNode::prepare(double sr, int spb)
{
    RelativisticNode::prepare(sr, spb);
    cyclePhase = 0.0;
    cycleCount = 0;
    evaluateCurrentCycle();
}

void TidalSeqNode::process(int numSamples)
{
    const auto& timeIn = getInletTimeFrame(1);
    double currentGamma = timeIn.masterGamma > 0.0001 ? timeIn.masterGamma : 1.0;

    auto& freqOut = getOutletBuffer(1);     // Outlet 1: freqOut~
    auto& audioTrig = getOutletBuffer(4);   // Outlet 4: audioTrig~

    for (int i = 0; i < numSamples; ++i)
    {
        double dt = (1.0 / currentSampleRate) * currentGamma;
        double dPhase = dt / cycleDurationSec;

        cyclePhase += dPhase;

        float sampleTrig = 0.0f;

        // Check if cycle rolled over
        if (cyclePhase >= 1.0)
        {
            cyclePhase -= 1.0;
            cycleCount++;
            evaluateCurrentCycle();
        }

        // Fire events whose startCycle is crossed in this sample interval
        while (nextEventIdx < scheduledEvents.size() && scheduledEvents[nextEventIdx].startCycle <= cyclePhase)
        {
            const auto& ev = scheduledEvents[nextEventIdx];
            if (!ev.isRest)
            {
                sampleTrig = 1.0f;

                if (ev.channel == 0)
                {
                    emitMessageOnOutlet(0, std::to_string(ev.pitch));
                    emitMessageOnOutlet(2, "bang");

                    double freqHz = 440.0 * std::pow(2.0, (ev.pitch - 69.0) / 12.0);
                    if (i < freqOut.getNumSamples())
                    {
                        freqOut.setSample(0, i, static_cast<float>(freqHz));
                    }
                }
                else
                {
                    emitMessageOnOutlet(3, std::to_string(ev.pitch));
                }
            }

            nextEventIdx++;
        }

        if (i < audioTrig.getNumSamples())
        {
            audioTrig.setSample(0, i, sampleTrig);
        }
    }
}

void TidalSeqNode::receiveMessage(const std::string& message)
{
    std::istringstream iss(message);
    std::string cmd;
    iss >> cmd;

    if (cmd == "pattern" || cmd == "pat" || cmd == "set" || cmd == "seq.tidal" || cmd == "tidal")
    {
        std::string pat;
        std::getline(iss, pat);
        size_t first = pat.find_first_not_of(" \t");
        if (first != std::string::npos) pat = pat.substr(first);
        if (!pat.empty()) setPattern(pat);
    }
    else if (cmd == "cps" || cmd == "speed" || cmd == "dur")
    {
        double d;
        if (iss >> d) setCycleDuration(d);
    }
    else if (cmd == "reset")
    {
        cyclePhase = 0.0;
        cycleCount = 0;
        evaluateCurrentCycle();
    }
    else
    {
        if (!message.empty())
        {
            setPattern(message);
        }
    }
}

} // namespace TimeDilationDAW
