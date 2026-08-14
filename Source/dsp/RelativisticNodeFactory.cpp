#include "RelativisticNodeFactory.h"
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

    // Default fallback to osc~
    return std::make_shared<OscNode>(nodeId, "sin");
}

} // namespace TimeDilationDAW
