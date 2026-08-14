#include "RelativisticSequencerNodes.h"
#include <sstream>
#include <random>

namespace TimeDilationDAW
{

// ==============================================================================
// 1. EuclidSequencerNode Implementation
// ==============================================================================
EuclidSequencerNode::EuclidSequencerNode(int id, int k, int n, int rot)
    : RelativisticNode(id, "seq.euclid", "Euclidean Rhythm Generator")
{
    // Ports:
    // Inlets: 0: msgIn, 1: timeIn (TimeFrame), 2: k_pulses (Msg), 3: n_steps (Msg)
    addInlet("timeIn", PortDataType::Time);
    addInlet("kIn", PortDataType::Message);
    addInlet("nIn", PortDataType::Message);

    // Outlets: 0: msgOut, 1: trigGate (Msg), 2: stepNum (Msg), 3: audioTrig~ (Audio)
    addOutlet("trigGate", PortDataType::Message);
    addOutlet("stepNum", PortDataType::Message);
    addOutlet("audioTrig", PortDataType::Audio);

    setParams(k, n, rot, 0.0f);
}

void EuclidSequencerNode::generateBjorklundPattern()
{
    pattern.clear();
    if (totalSteps <= 0) totalSteps = 8;
    int k = std::clamp(pulses, 0, totalSteps);
    int n = totalSteps;

    if (k == 0)
    {
        pattern.assign(static_cast<size_t>(n), false);
        return;
    }
    if (k >= n)
    {
        pattern.assign(static_cast<size_t>(n), true);
        return;
    }

    // Bjorklund's algorithm using vector groups
    std::vector<std::vector<bool>> groups;
    for (int i = 0; i < n; ++i)
    {
        groups.push_back({ i < k });
    }

    int countFalse = n - k;
    int countTrue = k;

    while (countFalse > 0)
    {
        int minCount = std::min(countTrue, countFalse);
        for (int i = 0; i < minCount; ++i)
        {
            auto& backGroup = groups.back();
            groups[static_cast<size_t>(i)].insert(groups[static_cast<size_t>(i)].end(), backGroup.begin(), backGroup.end());
            groups.pop_back();
        }
        if (countTrue > countFalse)
        {
            countTrue -= countFalse;
        }
        else
        {
            countFalse -= countTrue;
        }
    }

    for (const auto& g : groups)
    {
        pattern.insert(pattern.end(), g.begin(), g.end());
    }

    // Apply rotation
    if (!pattern.empty() && rotation != 0)
    {
        int rot = (rotation % static_cast<int>(pattern.size()) + static_cast<int>(pattern.size())) % static_cast<int>(pattern.size());
        std::rotate(pattern.begin(), pattern.begin() + rot, pattern.end());
    }
}

void EuclidSequencerNode::setParams(int k, int n, int rot, float sw)
{
    pulses = std::max(0, k);
    totalSteps = std::max(1, n);
    rotation = rot;
    swing = std::clamp(sw, 0.0f, 0.75f);
    currentStep = 0;
    generateBjorklundPattern();
}

void EuclidSequencerNode::prepare(double sr, int spb)
{
    RelativisticNode::prepare(sr, spb);
    accumulatedProperTime = 0.0;
    currentStep = 0;
}

void EuclidSequencerNode::process(int numSamples)
{
    const auto& timeIn = getInletTimeFrame(0);
    double currentGamma = timeIn.masterGamma > 0.0001 ? timeIn.masterGamma : 1.0;
    auto& audioOut = getOutletBuffer(2); // Outlet 2: audioTrig~

    for (int i = 0; i < numSamples; ++i)
    {
        double dt = (1.0 / currentSampleRate) * currentGamma;
        accumulatedProperTime += dt;

        float swingOffset = (currentStep % 2 == 1) ? (swing * 0.5f * static_cast<float>(stepDurationSec)) : 0.0f;
        double effectiveDuration = stepDurationSec + swingOffset;

        float samplePulse = 0.0f;

        if (accumulatedProperTime >= effectiveDuration)
        {
            accumulatedProperTime -= effectiveDuration;
            bool hit = !pattern.empty() && pattern[static_cast<size_t>(currentStep % static_cast<int>(pattern.size()))];

            if (hit)
            {
                samplePulse = 1.0f;
                // Dispatch trigger bang message on outlet 0
                emitMessageOnOutlet(0, "bang");
            }

            emitMessageOnOutlet(1, std::to_string(currentStep));

            currentStep = (currentStep + 1) % std::max(1, static_cast<int>(pattern.size()));
        }

        if (i < audioOut.getNumSamples())
        {
            audioOut.setSample(0, i, samplePulse);
        }
    }
}

void EuclidSequencerNode::receiveMessage(const std::string& message)
{
    std::istringstream iss(message);
    std::string cmd;
    iss >> cmd;

    if (cmd == "params" || cmd == "set")
    {
        int k = pulses, n = totalSteps, rot = rotation;
        float sw = swing;
        iss >> k >> n >> rot >> sw;
        setParams(k, n, rot, sw);
    }
    else if (cmd == "pulses" || cmd == "k")
    {
        int k;
        if (iss >> k) setParams(k, totalSteps, rotation, swing);
    }
    else if (cmd == "steps" || cmd == "n")
    {
        int n;
        if (iss >> n) setParams(pulses, n, rotation, swing);
    }
    else if (cmd == "rotate" || cmd == "rot")
    {
        int rot;
        if (iss >> rot) setParams(pulses, totalSteps, rot, swing);
    }
    else if (cmd == "swing")
    {
        float sw;
        if (iss >> sw) setParams(pulses, totalSteps, rotation, sw);
    }
    else if (cmd == "reset")
    {
        currentStep = 0;
        accumulatedProperTime = 0.0;
    }
}

// ==============================================================================
// 2. ArpNode Implementation
// ==============================================================================
ArpNode::ArpNode(int id, ArpMode m, int oct, double rate)
    : RelativisticNode(id, "seq.arp", "Relativistic Arpeggiator")
    , mode(m), octaves(oct), rateSec(rate)
{
    addInlet("timeIn", PortDataType::Time);
    addInlet("chordIn", PortDataType::Message);

    addOutlet("noteOut", PortDataType::Message);
    addOutlet("freqOut", PortDataType::Audio);

    rebuildNoteList();
}

void ArpNode::setChordNotes(const std::vector<int>& notes)
{
    inputNotes = notes;
    rebuildNoteList();
}

void ArpNode::rebuildNoteList()
{
    generatedNotes.clear();
    if (inputNotes.empty()) return;

    std::vector<int> sorted = inputNotes;
    if (mode != ArpMode::AsPlayed)
    {
        std::sort(sorted.begin(), sorted.end());
    }

    for (int oct = 0; oct < octaves; ++oct)
    {
        for (int note : sorted)
        {
            generatedNotes.push_back(note + oct * 12);
        }
    }

    if (mode == ArpMode::Down)
    {
        std::reverse(generatedNotes.begin(), generatedNotes.end());
    }

    currentIndex = 0;
    pingPongDirectionUp = true;
}

void ArpNode::prepare(double sr, int spb)
{
    RelativisticNode::prepare(sr, spb);
    accumulatedProperTime = 0.0;
    currentIndex = 0;
}

void ArpNode::process(int numSamples)
{
    const auto& timeIn = getInletTimeFrame(0);
    double currentGamma = timeIn.masterGamma > 0.0001 ? timeIn.masterGamma : 1.0;
    auto& freqOut = getOutletBuffer(1); // Outlet 1: freqOut~

    for (int i = 0; i < numSamples; ++i)
    {
        double dt = (1.0 / currentSampleRate) * currentGamma;
        accumulatedProperTime += dt;

        if (accumulatedProperTime >= rateSec)
        {
            accumulatedProperTime -= rateSec;

            if (!generatedNotes.empty())
            {
                int currentNote = 60;

                if (mode == ArpMode::Random)
                {
                    static std::mt19937 rng(1337);
                    std::uniform_int_distribution<size_t> dist(0, generatedNotes.size() - 1);
                    currentNote = generatedNotes[dist(rng)];
                }
                else if (mode == ArpMode::PingPong)
                {
                    currentNote = generatedNotes[static_cast<size_t>(currentIndex)];
                    if (pingPongDirectionUp)
                    {
                        if (currentIndex + 1 >= static_cast<int>(generatedNotes.size()))
                        {
                            pingPongDirectionUp = false;
                            currentIndex = std::max(0, currentIndex - 1);
                        }
                        else ++currentIndex;
                    }
                    else
                    {
                        if (currentIndex - 1 < 0)
                        {
                            pingPongDirectionUp = true;
                            currentIndex = std::min(static_cast<int>(generatedNotes.size()) - 1, currentIndex + 1);
                        }
                        else --currentIndex;
                    }
                }
                else
                {
                    currentNote = generatedNotes[static_cast<size_t>(currentIndex)];
                    currentIndex = (currentIndex + 1) % static_cast<int>(generatedNotes.size());
                }

                emitMessageOnOutlet(0, std::to_string(currentNote));

                // Send Hz signal on outlet 1
                double freqHz = 440.0 * std::pow(2.0, (currentNote - 69.0) / 12.0);
                if (i < freqOut.getNumSamples())
                {
                    freqOut.setSample(0, i, static_cast<float>(freqHz));
                }
            }
        }
    }
}

void ArpNode::receiveMessage(const std::string& message)
{
    std::istringstream iss(message);
    std::string cmd;
    iss >> cmd;

    if (cmd == "chord" || cmd == "notes")
    {
        std::vector<int> notes;
        int n;
        while (iss >> n) notes.push_back(n);
        if (!notes.empty()) setChordNotes(notes);
    }
    else if (cmd == "mode")
    {
        std::string mStr;
        iss >> mStr;
        if (mStr == "up") setMode(ArpMode::Up);
        else if (mStr == "down") setMode(ArpMode::Down);
        else if (mStr == "pingpong" || mStr == "updown") setMode(ArpMode::PingPong);
        else if (mStr == "random") setMode(ArpMode::Random);
        else if (mStr == "asplayed" || mStr == "order") setMode(ArpMode::AsPlayed);
    }
    else if (cmd == "octaves" || cmd == "oct")
    {
        int oct;
        if (iss >> oct) setOctaves(oct);
    }
    else if (cmd == "rate" || cmd == "speed")
    {
        double r;
        if (iss >> r) setRate(r);
    }
}

// ==============================================================================
// 3. PolySeqNode Implementation
// ==============================================================================
PolySeqNode::PolySeqNode(int id)
    : RelativisticNode(id, "seq.poly", "Polyrhythmic Multi-Meter Sequencer")
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("lane1", PortDataType::Message);
    addOutlet("lane2", PortDataType::Message);
    addOutlet("lane3", PortDataType::Message);

    lanes.resize(3);
    lanes[0].pitches = { 60, 64, 67 };          // 3-step lane
    lanes[1].pitches = { 48, 51, 55, 58 };      // 4-step lane
    lanes[2].pitches = { 72, 74, 76, 79, 81 };  // 5-step lane
}

void PolySeqNode::setLane(int laneIdx, const std::vector<int>& pitches)
{
    if (laneIdx >= 0 && laneIdx < static_cast<int>(lanes.size()))
    {
        lanes[static_cast<size_t>(laneIdx)].pitches = pitches;
        lanes[static_cast<size_t>(laneIdx)].currentStep = 0;
    }
}

void PolySeqNode::prepare(double sr, int spb)
{
    RelativisticNode::prepare(sr, spb);
    accumulatedProperTime = 0.0;
    for (auto& lane : lanes) lane.currentStep = 0;
}

void PolySeqNode::process(int numSamples)
{
    const auto& timeIn = getInletTimeFrame(0);
    double currentGamma = timeIn.masterGamma > 0.0001 ? timeIn.masterGamma : 1.0;

    for (int i = 0; i < numSamples; ++i)
    {
        double dt = (1.0 / currentSampleRate) * currentGamma;
        accumulatedProperTime += dt;

        if (accumulatedProperTime >= baseStepDuration)
        {
            accumulatedProperTime -= baseStepDuration;

            for (size_t l = 0; l < lanes.size(); ++l)
            {
                auto& lane = lanes[l];
                if (!lane.pitches.empty())
                {
                    int note = lane.pitches[static_cast<size_t>(lane.currentStep)];
                    emitMessageOnOutlet(static_cast<int>(l), std::to_string(note));
                    lane.currentStep = (lane.currentStep + 1) % static_cast<int>(lane.pitches.size());
                }
            }
        }
    }
}

void PolySeqNode::receiveMessage(const std::string& message)
{
    std::istringstream iss(message);
    std::string cmd;
    iss >> cmd;

    if (cmd == "lane" || cmd == "l")
    {
        int laneIdx;
        iss >> laneIdx;
        std::vector<int> notes;
        int n;
        while (iss >> n) notes.push_back(n);
        if (!notes.empty()) setLane(laneIdx - 1, notes);
    }
    else if (cmd == "rate")
    {
        double r;
        if (iss >> r) baseStepDuration = std::max(0.01, r);
    }
}

// ==============================================================================
// 4. TimelineAutomationNode Implementation
// ==============================================================================
TimelineAutomationNode::TimelineAutomationNode(int id, float defaultValue)
    : RelativisticNode(id, "auto~", "Timeline Parameter Automation Reader")
{
    addInlet("timeIn", PortDataType::Time);
    addOutlet("signalOut", PortDataType::Audio);
    addOutlet("msgOut", PortDataType::Message);

    currentValue.store(defaultValue);
    addBreakpoint(0.0, defaultValue);
}

void TimelineAutomationNode::clearBreakpoints()
{
    breakpoints.clear();
}

void TimelineAutomationNode::addBreakpoint(double timeSec, float value)
{
    breakpoints.push_back({ timeSec, value });
    std::sort(breakpoints.begin(), breakpoints.end(), [](const Breakpoint& a, const Breakpoint& b) {
        return a.timeSec < b.timeSec;
    });
}

float TimelineAutomationNode::evaluateAt(double timeSec) const
{
    if (breakpoints.empty()) return currentValue.load();
    if (breakpoints.size() == 1 || timeSec <= breakpoints.front().timeSec) return breakpoints.front().value;
    if (timeSec >= breakpoints.back().timeSec) return breakpoints.back().value;

    for (size_t i = 0; i < breakpoints.size() - 1; ++i)
    {
        if (timeSec >= breakpoints[i].timeSec && timeSec <= breakpoints[i + 1].timeSec)
        {
            double t0 = breakpoints[i].timeSec;
            double t1 = breakpoints[i + 1].timeSec;
            float v0 = breakpoints[i].value;
            float v1 = breakpoints[i + 1].value;

            double u = (timeSec - t0) / (t1 - t0);
            // C2 Smoothstep Hermite curve
            double h = 3.0 * u * u - 2.0 * u * u * u;
            return static_cast<float>(v0 + (v1 - v0) * h);
        }
    }

    return breakpoints.back().value;
}

void TimelineAutomationNode::prepare(double sr, int spb)
{
    RelativisticNode::prepare(sr, spb);
    accumulatedProperTime = 0.0;
}

void TimelineAutomationNode::process(int numSamples)
{
    const auto& timeIn = getInletTimeFrame(0);
    double currentGamma = timeIn.masterGamma > 0.0001 ? timeIn.masterGamma : 1.0;
    auto& sigOut = getOutletBuffer(0); // Outlet 0: signalOut~

    for (int i = 0; i < numSamples; ++i)
    {
        double dt = (1.0 / currentSampleRate) * currentGamma;
        accumulatedProperTime += dt;

        float val = evaluateAt(accumulatedProperTime);
        currentValue.store(val);

        if (i < sigOut.getNumSamples())
        {
            sigOut.setSample(0, i, val);
        }
    }
}

void TimelineAutomationNode::receiveMessage(const std::string& message)
{
    std::istringstream iss(message);
    std::string cmd;
    iss >> cmd;

    if (cmd == "point" || cmd == "add")
    {
        double t;
        float v;
        if (iss >> t >> v) addBreakpoint(t, v);
    }
    else if (cmd == "clear")
    {
        clearBreakpoints();
    }
    else if (cmd == "val" || cmd == "set")
    {
        float v;
        if (iss >> v) { currentValue.store(v); addBreakpoint(accumulatedProperTime, v); }
    }
}

} // namespace TimeDilationDAW
