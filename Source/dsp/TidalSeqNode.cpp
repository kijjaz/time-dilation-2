#include "TidalSeqNode.h"
#include <sstream>

namespace TimeDilationDAW
{

TidalSeqNode::TidalSeqNode(int id, const std::string& patternString, double cycleDur)
    : RelativisticNode(id, "seq.tidal", "seq.tidal " + patternString)
    , cycleDurationSec(cycleDur)
{
    // Inlets:
    // 0: msgIn (set pattern / speed / dur), 1: timeIn (TimeFrame)
    addInlet("msgIn", PortDataType::Message);
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
    auto& freqOut = getOutletBuffer(1);     // Outlet 1: freqOut~
    auto& audioTrig = getOutletBuffer(4);   // Outlet 4: audioTrig~

    // Only advance when connected to an active time/clock stream (e.g. time.transport~ / timeline~)!
    bool isDriven = false;
    TimePolyFrame timeIn;
    for (size_t idx = 0; idx < getInlets().size(); ++idx)
    {
        if (getInlets()[idx].dataType == PortDataType::Time && isInletConnected(static_cast<int>(idx)))
        {
            isDriven = true;
            timeIn = getInletTimeFrame(static_cast<int>(idx));
            break;
        }
    }
    // Fallback if connected to inlet 0
    if (!isDriven && isInletConnected(0))
    {
        isDriven = true;
        timeIn = getInletTimeFrame(0);
    }

    const bool hasSampleGamma = (timeIn.sampleGamma.size() >= static_cast<size_t>(numSamples));
    double masterG = isDriven ? std::max(0.0, timeIn.masterGamma) : 0.0;

    if (!isDriven || (masterG <= 0.000001 && !hasSampleGamma))
    {
        freqOut.clear();
        audioTrig.clear();
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        double currentGamma = hasSampleGamma ? std::max(0.0, static_cast<double>(timeIn.sampleGamma[static_cast<size_t>(i)])) : masterG;
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
