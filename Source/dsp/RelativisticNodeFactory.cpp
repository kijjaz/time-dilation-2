#include "RelativisticNodeFactory.h"
#include "PrintNode.h"
#include "PdControlNodes.h"
#include "PdSampleNodes.h"
#include "PdDelayNodes.h"
#include "RelativisticTimeNodes.h"
#include "RelativisticSequencerNodes.h"
#include <sstream>
#include <vector>

namespace TimeDilationDAW
{

std::shared_ptr<RelativisticNode> RelativisticNodeFactory::createNode(int nodeId, const std::string& symbolAndArgs)
{
    std::stringstream ss(symbolAndArgs);
    std::string symbol;
    ss >> symbol;

    if (symbol == "osc~")
    {
        std::string wave = "sin";
        if (ss >> wave) {}
        return std::make_shared<OscNode>(nodeId, wave);
    }
    else if (symbol == "osc.patch~" || symbol == "osc.composite~")
    {
        std::string wave = "sin";
        if (ss >> wave) {}
        auto compNode = std::make_shared<CompositeNode>(nodeId, "osc.patch~", "osc.patch~ " + wave);
        auto& subGraph = compNode->getInternalSubGraph();
        subGraph.clearGraph();

        // Create internal inspectable components inside osc.patch~
        auto oscCore = RelativisticNodeFactory::createNode(1, "osc~ " + wave);
        oscCore->xPos = 100; oscCore->yPos = 100;

        auto ladder = RelativisticNodeFactory::createNode(2, "ladder~ 2500 0.4");
        ladder->xPos = 270; ladder->yPos = 100;

        auto outAudio = RelativisticNodeFactory::createNode(3, "out~");
        outAudio->xPos = 440; outAudio->yPos = 100;

        subGraph.addNode(oscCore);
        subGraph.addNode(ladder);
        subGraph.addNode(outAudio);

        subGraph.addConnection(1, 1, 2, 1); // oscCore out~ (Outlet 1) -> ladder in~ (Inlet 1)
        subGraph.addConnection(2, 1, 3, 1); // ladder out~ (Outlet 1) -> out~ in~ (Inlet 1)

        return compNode;
    }
    else if (symbol == "table")
    {
        std::string name = "array1";
        size_t sz = 44100;
        if (ss >> name) {}
        if (ss >> sz) {}
        return std::make_shared<TableNode>(nodeId, name, sz);
    }
    else if (symbol == "tabread~")
    {
        std::string name = "array1";
        if (ss >> name) {}
        return std::make_shared<TabReadTildeNode>(nodeId, name);
    }
    else if (symbol == "svf~")
    {
        double cut = 1000.0, q = 0.707;
        if (ss >> cut) {}
        if (ss >> q) {}
        return std::make_shared<SVFNode>(nodeId, cut, q);
    }
    else if (symbol == "ladder~")
    {
        double cut = 1000.0, res = 0.5;
        if (ss >> cut) {}
        if (ss >> res) {}
        return std::make_shared<LadderNode>(nodeId, cut, res);
    }
    else if (symbol == "drive~" || symbol == "saturate~")
    {
        double d = 2.0;
        if (ss >> d) {}
        return std::make_shared<DriveNode>(nodeId, d);
    }
    else if (symbol == "pluck~")
    {
        double p = 220.0;
        if (ss >> p) {}
        return std::make_shared<PluckNode>(nodeId, p);
    }
    else if (symbol == "msg" || symbol == "message")
    {
        std::string text;
        std::getline(ss, text);
        if (!text.empty() && text[0] == ' ') text = text.substr(1);
        return std::make_shared<MessageNode>(nodeId, text.empty() ? "set 440" : text);
    }
    else if (symbol == "delay~")
    {
        double maxSec = 2.0;
        if (ss >> maxSec) {}
        return std::make_shared<DelayNode>(nodeId, maxSec);
    }
    else if (symbol == "out~")
    {
        return std::make_shared<OutNode>(nodeId);
    }
    else if (symbol == "bang" || symbol == "bng")
    {
        return std::make_shared<BangNode>(nodeId);
    }
    else if (symbol == "toggle" || symbol == "tgl")
    {
        bool init = false;
        std::string arg;
        if (ss >> arg)
        {
            if (arg == "1" || arg == "on" || arg == "true" || arg == "[x]") init = true;
        }
        return std::make_shared<ToggleNode>(nodeId, init);
    }
    else if (symbol == "number" || symbol == "num")
    {
        double val = 0.0;
        if (ss >> val) {}
        return std::make_shared<NumberNode>(nodeId, val);
    }
    else if (symbol == "symbol" || symbol == "sym")
    {
        std::string text = "symbol";
        if (ss >> text) {}
        return std::make_shared<SymbolNode>(nodeId, text);
    }
    else if (symbol == "radio" || symbol == "hradio" || symbol == "vradio")
    {
        int opts = 4, sel = 0;
        if (ss >> opts) {}
        if (ss >> sel) {}
        return std::make_shared<RadioNode>(nodeId, opts, sel);
    }
    else if (symbol == "display" || symbol == "disp" || symbol == "print")
    {
        return std::make_shared<DisplayNode>(nodeId);
    }
    else if (symbol == "time.transport~" || symbol == "time.transport" || symbol == "transport~" || symbol == "transport")
    {
        return std::make_shared<TransportNode>(nodeId);
    }
    else if (symbol == "time.scope~" || symbol == "time.scope")
    {
        return std::make_shared<TimeScopeNode>(nodeId);
    }
    else if (symbol == "time.lfo~" || symbol == "time.lfo")
    {
        double r = 0.5, d = 0.8;
        if (ss >> r) {}
        if (ss >> d) {}
        return std::make_shared<TimeLFONode>(nodeId, r, d);
    }
    else if (symbol == "time.warp~" || symbol == "time.warp")
    {
        double f = 2.0;
        if (ss >> f) {}
        return std::make_shared<TimeWarpNode>(nodeId, f);
    }
    else if (symbol == "time.retro~" || symbol == "time.retro")
    {
        return std::make_shared<TimeRetroNode>(nodeId);
    }
    else if (symbol == "time.stasis~" || symbol == "time.stasis")
    {
        return std::make_shared<TimeStasisNode>(nodeId);
    }
    else if (symbol == "time.math~" || symbol == "time.math")
    {
        return std::make_shared<TimeMathNode>(nodeId);
    }
    else if (symbol == "seq")
    {
        std::string restOfLine;
        std::getline(ss, restOfLine);
        return std::make_shared<SeqNode>(nodeId, restOfLine);
    }
    else if (symbol == "mtof" || symbol == "mtof~")
    {
        return std::make_shared<MtofNode>(nodeId);
    }
    else if (symbol == "ftom" || symbol == "ftom~")
    {
        return std::make_shared<FtomNode>(nodeId);
    }
    else if (symbol == "time.grav.osc~" || symbol == "time.grav~" || symbol == "grav.osc~" || symbol == "grav.osc")
    {
        double m = 1.0, r = 2.0;
        if (ss >> m) {}
        if (ss >> r) {}
        return std::make_shared<GravRedshiftOscNode>(nodeId, m, r);
    }
    else if (symbol == "time.lorentz~" || symbol == "time.lorentz" || symbol == "lorentz~" || symbol == "lorentz.filter~")
    {
        double cut = 1200.0, v = 0.5;
        if (ss >> cut) {}
        if (ss >> v) {}
        return std::make_shared<LorentzWarpFilterNode>(nodeId, cut, v);
    }
    else if (symbol == "time.tachyon.grain~" || symbol == "time.tachyon~" || symbol == "tachyon.grain~" || symbol == "tachyon~")
    {
        double dur = 50.0;
        if (ss >> dur) {}
        return std::make_shared<TachyonGranularNode>(nodeId, dur);
    }
    else if (symbol == "meter~" || symbol == "vu~")
    {
        std::string modeStr = "peak";
        if (ss >> modeStr) {}
        MeterNode::MeterMode m = MeterNode::MeterMode::Peak;
        if (modeStr == "rms") m = MeterNode::MeterMode::RMS;
        else if (modeStr == "lufs") m = MeterNode::MeterMode::LUFS;
        return std::make_shared<MeterNode>(nodeId, m);
    }
    else if (symbol == "spectrogram~" || symbol == "spec~")
    {
        return std::make_shared<SpectrogramNode>(nodeId);
    }
    else if (symbol == "pack~" || symbol == "bundle~" || symbol == "join~" || symbol == "snake.in~" || symbol == "mc.combine~")
    {
        int chs = 2;
        if (ss >> chs) {}
        return std::make_shared<PackNode>(nodeId, chs);
    }
    else if (symbol == "unpack~" || symbol == "unbundle~" || symbol == "split~" || symbol == "snake.out~" || symbol == "mc.separate~")
    {
        int chs = 2;
        if (ss >> chs) {}
        return std::make_shared<UnpackNode>(nodeId, chs);
    }
    else if (symbol == "reverb~" || symbol == "freeverb~" || symbol == "rev1~")
    {
        float room = 0.7f, damp = 0.4f, wet = 0.35f;
        if (ss >> room) {}
        if (ss >> damp) {}
        if (ss >> wet) {}
        return std::make_shared<ReverbNode>(nodeId, room, damp, wet);
    }
    else if (symbol == "noise~")
    {
        return std::make_shared<NoiseNode>(nodeId);
    }
    else if (symbol == "kick~" || symbol == "drum.kick~" || symbol == "kick")
    {
        double pitch = 50.0, dec = 0.35;
        if (ss >> pitch) {}
        if (ss >> dec) {}
        return std::make_shared<KickNode>(nodeId, pitch, dec);
    }
    else if (symbol == "snare~" || symbol == "drum.snare~" || symbol == "snare")
    {
        double tone = 185.0, snap = 0.65, dec = 0.28;
        if (ss >> tone) {}
        if (ss >> snap) {}
        if (ss >> dec) {}
        return std::make_shared<SnareNode>(nodeId, tone, snap, dec);
    }
    else if (symbol == "hihat~" || symbol == "drum.hat~" || symbol == "hat~" || symbol == "hihat")
    {
        double dec = 0.08;
        if (ss >> dec) {}
        return std::make_shared<HiHatNode>(nodeId, dec);
    }
    else if (symbol == "patch~")
    {
        std::string patchName = "synth.voice~";
        if (ss >> patchName) {}
        return std::make_shared<CompositeNode>(nodeId, "patch~", patchName);
    }
    else if (symbol == "print")
    {
        std::string prefix = "print";
        if (ss >> prefix) {}
        return std::make_shared<PrintNode>(nodeId, prefix, false);
    }
    else if (symbol == "print~")
    {
        std::string prefix = "print~";
        if (ss >> prefix) {}
        return std::make_shared<PrintNode>(nodeId, prefix, true);
    }

    else if (symbol == "trigger" || symbol == "t")
    {
        std::vector<std::string> types;
        std::string arg;
        while (ss >> arg)
        {
            types.push_back(arg);
        }
        if (types.empty()) types = { "b", "b" };
        return std::make_shared<TriggerNode>(nodeId, types);
    }
    else if (symbol == "select" || symbol == "sel")
    {
        std::vector<std::string> targets;
        std::string arg;
        while (ss >> arg)
        {
            targets.push_back(arg);
        }
        if (targets.empty()) targets = { "0" };
        return std::make_shared<SelectNode>(nodeId, targets);
    }
    else if (symbol == "route")
    {
        std::vector<std::string> selectors;
        std::string arg;
        while (ss >> arg)
        {
            selectors.push_back(arg);
        }
        if (selectors.empty()) selectors = { "pitch" };
        return std::make_shared<RouteNode>(nodeId, selectors);
    }
    else if (symbol == "line~" || symbol == "ramp~")
    {
        double initVal = 0.0;
        if (ss >> initVal) {}
        return std::make_shared<LineTildeNode>(nodeId, initVal);
    }
    else if (symbol == "metro")
    {
        double interval = 500.0;
        if (ss >> interval) {}
        return std::make_shared<MetroNode>(nodeId, interval);
    }
    else if (symbol == "del" || symbol == "delay")
    {
        double dMs = 100.0;
        if (ss >> dMs) {}
        return std::make_shared<DelNode>(nodeId, dMs);
    }
    else if (symbol == "random")
    {
        int maxVal = 10;
        if (ss >> maxVal) {}
        return std::make_shared<RandomNode>(nodeId, maxVal);
    }
    else if (symbol == "counter")
    {
        int minV = 0, maxV = 16, step = 1;
        if (ss >> minV) {}
        if (ss >> maxV) {}
        if (ss >> step) {}
        return std::make_shared<CounterNode>(nodeId, minV, maxV, step);
    }
    else if (symbol == "soundfiler")
    {
        return std::make_shared<SoundfilerNode>(nodeId);
    }
    else if (symbol == "readsf~")
    {
        int chs = 2;
        if (ss >> chs) {}
        return std::make_shared<ReadSFTildeNode>(nodeId, chs);
    }
    else if (symbol == "delwrite~")
    {
        std::string name = "del1";
        double maxMs = 1000.0;
        if (ss >> name) {}
        if (ss >> maxMs) {}
        return std::make_shared<DelwriteTildeNode>(nodeId, name, maxMs);
    }
    else if (symbol == "delread~")
    {
        std::string name = "del1";
        double dMs = 100.0;
        if (ss >> name) {}
        if (ss >> dMs) {}
        return std::make_shared<DelreadTildeNode>(nodeId, name, dMs);
    }
    else if (symbol == "vd~" || symbol == "time.vd~")
    {
        std::string name = "del1";
        double dMs = 100.0;
        if (ss >> name) {}
        if (ss >> dMs) {}
        return std::make_shared<VdTildeNode>(nodeId, name, dMs);
    }
    else if (symbol == "pipe" || symbol == "time.pipe")
    {
        double dMs = 100.0;
        if (ss >> dMs) {}
        return std::make_shared<PipeNode>(nodeId, dMs);
    }
    else if (symbol == "timer" || symbol == "time.timer")
    {
        return std::make_shared<TimerNode>(nodeId);
    }
    else if (symbol == "snapshot~" || symbol == "time.snapshot~")
    {
        return std::make_shared<SnapshotTildeNode>(nodeId);
    }
    else if (symbol == "time.quantize" || symbol == "quantize")
    {
        double divMs = 125.0;
        if (ss >> divMs) {}
        return std::make_shared<TimeQuantizeNode>(nodeId, divMs);
    }
    else if (symbol == "time.const~" || symbol == "time.speed~")
    {
        double g = 1.0, tau = 0.0;
        if (ss >> g) {}
        if (ss >> tau) {}
        return std::make_shared<TimeConstNode>(nodeId, g, tau);
    }
    else if (symbol == "time.scale~" || symbol == "time.mul~")
    {
        double mult = 2.0, off = 0.0;
        if (ss >> mult) {}
        if (ss >> off) {}
        return std::make_shared<TimeScaleNode>(nodeId, mult, off);
    }
    else if (symbol == "time.add~")
    {
        double dg = 0.0, dtau = 0.0;
        if (ss >> dg) {}
        if (ss >> dtau) {}
        return std::make_shared<TimeAddNode>(nodeId, dg, dtau);
    }
    else if (symbol == "time.crossfade~" || symbol == "time.xfade~")
    {
        double mix = 0.5;
        if (ss >> mix) {}
        return std::make_shared<TimeCrossfadeNode>(nodeId, mix);
    }
    else if (symbol == "time.curve~" || symbol == "time.ramp~")
    {
        double g = 1.0, dur = 1000.0;
        if (ss >> g) {}
        if (ss >> dur) {}
        return std::make_shared<TimeCurveNode>(nodeId, g, dur);
    }
    else if (symbol == "time.chaos~")
    {
        double r = 0.5;
        std::string typeStr = "lorenz";
        if (ss >> r) {}
        if (ss >> typeStr) {}
        auto type = (typeStr == "rossler") ? TimeChaosNode::AttractorType::Rossler : TimeChaosNode::AttractorType::Lorenz;
        return std::make_shared<TimeChaosNode>(nodeId, r, type);
    }
    else if (symbol == "time.quantize~" || symbol == "time.grid~")
    {
        double divMs = 125.0, sw = 0.0;
        if (ss >> divMs) {}
        if (ss >> sw) {}
        return std::make_shared<TimeGridQuantizeNode>(nodeId, divMs, sw);
    }
    else if (symbol == "time.split~")
    {
        return std::make_shared<TimeSplitNode>(nodeId);
    }
    else if (symbol == "time.merge~")
    {
        return std::make_shared<TimeMergeNode>(nodeId);
    }
    else if (symbol == "seq.euclid" || symbol == "euclid")
    {
        int k = 3, n = 8, rot = 0;
        if (ss >> k) {}
        if (ss >> n) {}
        if (ss >> rot) {}
        return std::make_shared<EuclidSequencerNode>(nodeId, k, n, rot);
    }
    else if (symbol == "seq.arp" || symbol == "arp")
    {
        std::string modeStr = "up";
        int oct = 2;
        double rate = 0.125;
        if (ss >> modeStr) {}
        if (ss >> oct) {}
        if (ss >> rate) {}
        auto mode = ArpNode::ArpMode::Up;
        if (modeStr == "down") mode = ArpNode::ArpMode::Down;
        else if (modeStr == "pingpong" || modeStr == "updown") mode = ArpNode::ArpMode::PingPong;
        else if (modeStr == "random") mode = ArpNode::ArpMode::Random;
        else if (modeStr == "asplayed" || modeStr == "order") mode = ArpNode::ArpMode::AsPlayed;
        return std::make_shared<ArpNode>(nodeId, mode, oct, rate);
    }
    else if (symbol == "seq.poly")
    {
        return std::make_shared<PolySeqNode>(nodeId);
    }
    else if (symbol == "auto~" || symbol == "timeline.auto")
    {
        float defVal = 0.0f;
        if (ss >> defVal) {}
        return std::make_shared<TimelineAutomationNode>(nodeId, defVal);
    }

    // Default fallback to osc~
    return std::make_shared<OscNode>(nodeId, "sin");
}

} // namespace TimeDilationDAW
