#include "RelativisticCanvasComponent.h"
#include "CarbonGoldLookAndFeel.h"
#include "../dsp/CompositeNode.h"
#include "../dsp/RelativisticNodeFactory.h"
#include "../dsp/TidalSeqNode.h"
#include "../dsp/RelativisticSequencerNodes.h"

namespace TimeDilationDAW
{

RelativisticCanvasComponent::RelativisticCanvasComponent(RelativisticNodeGraph& graph)
    : rootGraph(graph)
{
    graphStack.push_back(&rootGraph);
    breadcrumbs.push_back("Master Patch");

    setWantsKeyboardFocus(true);

    objectEditor.setMultiLine(false);
    objectEditor.setColour(juce::TextEditor::backgroundColourId, CarbonGoldLookAndFeel::slatePanel);
    objectEditor.setColour(juce::TextEditor::textColourId, CarbonGoldLookAndFeel::goldAccent);
    objectEditor.setColour(juce::TextEditor::outlineColourId, CarbonGoldLookAndFeel::cyberCyan);
    objectEditor.setColour(juce::TextEditor::focusedOutlineColourId, CarbonGoldLookAndFeel::goldAccent);

    objectEditor.onReturnKey = [this]() { commitObjectCreation(); };
    objectEditor.onEscapeKey = [this]() { cancelObjectCreation(); };
    objectEditor.onTextChange = [this]() { updateAutocompleteSuggestions(); };

    addChildComponent(objectEditor);

    suggestionListBox.setModel(&autocompleteModel);
    suggestionListBox.setRowHeight(22);
    suggestionListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromRGB(0x0e, 0x12, 0x1c));
    suggestionListBox.setColour(juce::ListBox::outlineColourId, CarbonGoldLookAndFeel::goldAccent);
    suggestionListBox.setVisible(false);
    addChildComponent(suggestionListBox);

    allCatalogueObjects = {
        { "seq.tidal [60 [62 64] 67 [69 71 72]]", "TidalCycles Pattern Sequencer (Subdivisions, Polyphony, Euclids)", "tidal pattern seq strudel mini-notation rhythm groove livecoding polyphony subdivision" },
        { "seq.euclid 5 16", "Bjorklund Euclidean Rhythm Sequencer (5 pulses in 16 steps)", "euclid seq rhythm pattern bjorklund tresillo cinquillo" },
        { "seq.arp up 2 0.125", "Relativistic Chord Arpeggiator (up, down, updown, random)", "arp arpeggio seq pattern chord note melody" },
        { "seq.poly", "Polyrhythmic Multi-Meter Sequencer", "poly polyrhythm multi seq meter pattern rhythm polymeter" },
        { "seq notes 60 62 64 67", "Relativistic Step Sequencer", "step seq pattern note pitch sequencer" },
        { "auto~ 0.5", "Timeline Parameter Automation Reader (Spline interpolation)", "auto automation envelope curve spline parameter timeline" },
        { "time.curve~ 1.0 500", "Piecewise Time-Curvature Function Generator (Tape Stops, S-Curves)", "curve time tape stop s-curve warp slowdown" },
        { "time.chaos~ 0.5 lorenz", "Relativistic Chaotic Attractor Time Modulator (3D Lorenz / Rossler RK4)", "chaos lorenz rossler rk4 attractor time drift nonlinear" },
        { "time.crossfade~ 0.5", "Relativistic Spacetime Morpher & Dual Time Vector Interpolator", "crossfade morpher time blend dual vector" },
        { "time.const~ 1.0", "Constant / Stepped Speed Dilation (\u03b3 generator)", "const speed gamma time rate multiplier" },
        { "time.scale~ 2.0", "Time Multiplier & Polyrhythmic Divider", "scale time multiplier divider polyrhythm" },
        { "time.add~ 0.2", "Time Offset & Micro-Timing Groove Summer", "add offset microtiming tau delay swing groove" },
        { "time.quantize~ 125", "Continuous-to-Stepped Grid Quantizer", "quantize time grid step samplehold" },
        { "time.split~", "Time Frame to Audio Signal Splitter", "split timeframe audio channels demux" },
        { "time.merge~", "Audio Signals to Time Frame Merger", "merge timeframe audio channels mux" },
        { "time.lfo~ 0.5 0.8", "Relativistic Proper Time LFO (Rate: 0.5Hz, Depth: 0.8)", "lfo time rate modulation low frequency oscillator" },
        { "time.warp~ 1.5", "Relativistic Time Warp Node (\u03b3 = 1.5x speed)", "warp time relativistic speed dilation" },
        { "time.grav.osc~ 1.0 2.0", "Gravitational Redshift Oscillator (Mass: 1.0, Radius: 2.0)", "gravitational redshift black hole general relativity mass" },
        { "time.lorentz~ 1200 0.5", "Lorentz Velocity Filter (v = 0.5c)", "lorentz special relativity filter cutoff velocity" },
        { "time.tachyon.grain~ 50", "Faster-than-Light Granular Synthesizer (50ms grains)", "tachyon granular grain ftl clouds texture" },
        { "time.transport~", "Relativistic Transport Master Clock", "transport clock play tempo bpm sync" },
        { "time.scope~", "Proper Time Telemetry Plot", "scope telemetry visualizer gamma tau" },
        { "metro 125 1", "Relativistic Proper-Time Metronome & Clock", "metro clock pulse bang timer tick bpm" },
        { "counter 0 15 1", "Step Counter & Integer Clock Divider", "counter step divide integer count" },
        { "random 100", "Deterministic / Stochastic Integer Generator", "random chance stochastic dice noise" },
        { "select 0 4 8 12", "Value Matcher & Bang Dispatcher", "select sel match dispatch route" },
        { "route 1 2 3", "Prefix & Channel Router", "route prefix channel dispatch" },
        { "t b b", "Trigger Bangs in Right-to-Left Sequence", "trigger t bang sequence dispatch" },
        { "pipe 150", "Relativistic Proper-Time Timestamped Event Queue", "pipe delay queue event schedule" },
        { "timer", "Relativistic Proper-Time Stopwatch", "timer elapsed duration stopwatch delta" },
        { "snapshot~", "Instantaneous Audio & Time Frame Sampler", "snapshot sample hold meter probe" },
        { "delwrite~ del1 1000", "Relativistic Circular Delay Buffer Writer", "delwrite delay buffer write echo" },
        { "vd~ del1 150", "Relativistic Doppler Variable Delay Buffer Reader", "vd variable delay doppler pitch chorus flanger" },
        { "readsf~ 2", "Streaming Audio File Playback", "readsf read audio wave wav aif stream player sample" },
        { "soundfiler", "Audio File Reader & Table Buffer Ingestion", "soundfiler load wave wav sample table array" },
        { "spectrogram~", "Real-Time FFT Waterfall Spectrogram (0Hz - Nyquist)", "spectrogram spec fft waterfall spectrum visualizer analysis" },
        { "meter~", "Precision Level Meter (Peak, RMS, LUFS modes)", "meter vu lufs peak rms level volume visualizer" },
        { "osc~ sin 440", "Anti-aliased PolyBLEP Sine Oscillator @ 440 Hz", "osc sine oscillator synth tone pitch sound" },
        { "osc~ saw 220", "Sawtooth Oscillator @ 220 Hz", "osc saw sawtooth oscillator synth tone" },
        { "osc~ square 110", "Square Wave Oscillator @ 110 Hz", "osc square pulse oscillator synth tone" },
        { "osc~ tri 330", "Triangle Wave Oscillator @ 330 Hz", "osc tri triangle oscillator synth tone" },
        { "ladder~ 1500 0.5", "4-Pole Moog VA Ladder Filter (Cutoff: 1500Hz, Res: 0.5)", "ladder moog filter lowpass resonance cutoff va" },
        { "svf~ 1000 0.707", "State Variable Filter (Cutoff: 1000Hz, Q: 0.707)", "svf state variable filter bandpass highpass lowpass" },
        { "drive~ 2.0", "Hyperbolic WaveShaper Tube Distortion (Drive: 2.0)", "drive saturate distortion tube fuzz overdrive waveshaper" },
        { "pluck~ 220", "Karplus-Strong Physical String Model (Pitch: 220Hz)", "pluck karplus strong physical model guitar string harp" },
        { "reverb~ 0.7 0.4 0.35", "Stereo Algorithmic Reverberator (Room, Damp, Wet)", "reverb space room hall algorithmic ambience wet" },
        { "noise~", "White & Pink Noise Audio Generator", "noise white pink random hiss audio generator" },
        { "kick~ 50 0.35", "Analog Pitch-Sweep Sub-Bass Kick Drum", "kick bass drum 808 909 percussion bd" },
        { "snare~ 185 0.65 0.28", "Analog Dual-Tone & Filtered Noise Snare Drum", "snare drum noise percussion sn 808 909" },
        { "hihat~ 0.08", "Metallic Multi-Pulse Closed/Open Hi-Hat", "hihat hat closed open metallic percussion hh" },
        { "out~", "Master Stereo Output & Monitoring Node", "out dac master output speaker monitor" },
        { "table array1 44100", "Audio Sample Buffer Array (44100 samples)", "table array buffer memory sample ram" },
        { "tabread~ array1", "Audio Sample Buffer Reader", "tabread table read sample buffer player" },
        { "delay~ 2.0", "Feedback Delay Line (Max 2.0 seconds)", "delay echo feedback time repeat" },
        { "mtof 69", "MIDI Note Number to Frequency in Hz (e.g. 69 \u2192 440 Hz)", "mtof midi note frequency hz pitch convert" },
        { "ftom 440", "Frequency in Hz to MIDI Note Number (e.g. 440 Hz \u2192 69)", "ftom frequency hz midi note pitch convert" },
        { "pack~ 2", "Bundle N Mono Audio Inlets into 1 Multichannel Audio Cable", "pack bundle multichannel cable mux" },
        { "unpack~ 2", "Split 1 Multichannel Audio Cable into N Mono Outlets", "unpack split multichannel cable demux" },
        { "msg play", "Parameter Control Message Box ('play')", "msg message string command value" },
        { "msg cutoff 1200", "Parameter Control Message Box ('cutoff 1200')", "msg message parameter value command" }
    };

    recenterButton.setButtonText("Recenter View");
    recenterButton.onClick = [this]() { recenterView(); };
    addAndMakeVisible(recenterButton);

    startTimerHz(60); // 60 FPS real-time scope & canvas repainting
}

RelativisticCanvasComponent::~RelativisticCanvasComponent()
{
    stopTimer();
}

RelativisticNodeGraph& RelativisticCanvasComponent::getCurrentGraph()
{
    if (!graphStack.empty()) return *graphStack.back();
    return rootGraph;
}

void RelativisticCanvasComponent::pushSubGraphView(RelativisticNodeGraph* subGraph, const std::string& name)
{
    if (subGraph)
    {
        graphStack.push_back(subGraph);
        breadcrumbs.push_back(name);
        clearSelection();
        repaint();
    }
}

void RelativisticCanvasComponent::popSubGraphView()
{
    if (graphStack.size() > 1)
    {
        graphStack.pop_back();
        breadcrumbs.pop_back();
        clearSelection();
        repaint();
    }
}

static bool isControlGuiSymbol(const std::string& sym)
{
    return (sym == "msg" || sym == "message" || sym == "bang" || sym == "bng" ||
            sym == "toggle" || sym == "tgl" || sym == "number" || sym == "num" ||
            sym == "symbol" || sym == "sym" || sym == "radio" || sym == "hradio" ||
            sym == "vradio" || sym == "display" || sym == "disp" || sym == "print");
}

static bool isControlLogicSymbol(const std::string& sym)
{
    return (sym == "metro" || sym == "counter" || sym == "random" || sym == "select" ||
            sym == "route" || sym == "t" || sym == "trigger" || sym == "pipe" ||
            sym == "timer" || sym == "snapshot~" || sym == "mtof" || sym == "ftom" ||
            sym == "pack" || sym == "unpack" || sym == "soundfiler" || sym == "table");
}

static bool isSequencerSymbol(const std::string& sym)
{
    return (sym.rfind("seq", 0) == 0 || sym == "tidal" || sym == "pattern" || sym == "auto~");
}

static bool isTimeSculptorSymbol(const std::string& sym)
{
    return (sym.rfind("time.", 0) == 0);
}

juce::Rectangle<float> RelativisticCanvasComponent::getNodeBounds(const RelativisticNode& node) const
{
    std::string sym = node.getSymbol();
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

    float fontWidth = juce::Font(12.0f, juce::Font::bold).getStringWidthFloat(node.getLabel());
    float minWidthForPorts = static_cast<float>(std::max(node.getInlets().size(), node.getOutlets().size()) + 1) * 24.0f;

    if (isControlGuiSymbol(sym))
    {
        float w = std::max({ node.width, fontWidth + 24.0f, minWidthForPorts, 80.0f });
        return { node.xPos + viewOffsetX, node.yPos + viewOffsetY, w, 28.0f };
    }
    if (isControlLogicSymbol(sym))
    {
        float w = std::max({ node.width, fontWidth + 24.0f, minWidthForPorts, 90.0f });
        return { node.xPos + viewOffsetX, node.yPos + viewOffsetY, w, 32.0f };
    }
    if (isSequencerSymbol(sym))
    {
        float w = std::max({ node.width, fontWidth + 24.0f, minWidthForPorts, 170.0f });
        return { node.xPos + viewOffsetX, node.yPos + viewOffsetY, w, 52.0f };
    }

    // Audio or Time Sculptor nodes
    float calculatedW = std::max({ node.width, fontWidth + 85.0f, minWidthForPorts, 140.0f });
    float baseHeight = node.showRealtimeDisplay ? 85.0f : 32.0f;
    float calculatedH = std::max(node.height, baseHeight);

    return { node.xPos + viewOffsetX, node.yPos + viewOffsetY, calculatedW, calculatedH };
}

juce::Point<float> RelativisticCanvasComponent::getPortPos(const RelativisticNode& node, bool isOutlet, int portIdx) const
{
    auto b = getNodeBounds(node);
    const auto& ports = isOutlet ? node.getOutlets() : node.getInlets();
    int count = static_cast<int>(ports.size());
    float spacing = b.getWidth() / (count + 1);
    float x = b.getX() + spacing * (portIdx + 1);
    float y = isOutlet ? b.getBottom() : b.getY();
    return { x, y };
}

void RelativisticCanvasComponent::paint(juce::Graphics& g)
{
    auto& currGraph = getCurrentGraph();

    // Background Grid
    g.fillAll(CarbonGoldLookAndFeel::carbonBg);

    g.setColour(juce::Colour::fromRGB(0x1a, 0x1a, 0x24));
    for (int x = 0; x < getWidth(); x += 20)
        g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
    for (int y = 0; y < getHeight(); y += 20)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));

    // Breadcrumb Bar
    g.setColour(CarbonGoldLookAndFeel::slatePanel);
    g.fillRect(0, 0, getWidth(), 28);
    g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.3f));
    g.drawHorizontalLine(28, 0.0f, static_cast<float>(getWidth()));

    juce::String bcStr = "";
    for (size_t i = 0; i < breadcrumbs.size(); ++i)
    {
        if (i > 0) bcStr += "  >  ";
        bcStr += juce::String(breadcrumbs[i]);
    }
    g.setColour(CarbonGoldLookAndFeel::goldAccent);
    g.setFont(12.0f);
    g.drawText(bcStr, 10, 4, getWidth() - 20, 20, juce::Justification::left, true);

    // Render Patch Connections
    for (const auto& conn : currGraph.getConnections())
    {
        auto srcNode = currGraph.getNode(conn.sourceNodeId);
        auto destNode = currGraph.getNode(conn.destNodeId);
        if (!srcNode || !destNode) continue;

        auto p1 = getPortPos(*srcNode, true, conn.sourcePortIndex);
        auto p2 = getPortPos(*destNode, false, conn.destPortIndex);

        juce::Path cablePath;
        cablePath.startNewSubPath(p1);
        cablePath.cubicTo(p1.x, p1.y + 40.0f, p2.x, p2.y - 40.0f, p2.x, p2.y);

        bool isSelectedCable = (conn.connectionId == selectedConnectionId);
        juce::Colour typeColor = (conn.dataType == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                                ((conn.dataType == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);

        juce::Colour cableColor = conn.isFeedbackCycle ? juce::Colour::fromRGB(0xff, 0x44, 0x44)
                                                       : (isSelectedCable ? CarbonGoldLookAndFeel::goldAccent : typeColor);

        g.setColour(cableColor.withAlpha(0.35f));
        g.strokePath(cablePath, juce::PathStrokeType(5.0f));

        g.setColour(cableColor);
        g.strokePath(cablePath, juce::PathStrokeType(isSelectedCable ? 2.5f : 1.5f));

        // Real-Time Animated Kinetic Signal Pulse Dots & Flash Animations along the Cable!
        double millis = juce::Time::getMillisecondCounterHiRes();
        double seconds = millis * 0.001;

        if (conn.dataType == PortDataType::Message)
        {
            // Flash brightness pulse when a message passes through the cable!
            double age = seconds - conn.lastMessageTriggerTime;
            if (age >= 0.0 && age < 0.45)
            {
                float flashAlpha = 1.0f - static_cast<float>(age / 0.45);
                g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(flashAlpha * 0.8f));
                g.strokePath(cablePath, juce::PathStrokeType(4.0f));

                float pulsePos = static_cast<float>(age / 0.45);
                auto dot = cablePath.getPointAlongPath(pulsePos * cablePath.getLength());
                g.setColour(juce::Colours::white.withAlpha(flashAlpha));
                g.fillEllipse(dot.x - 4.0f, dot.y - 4.0f, 8.0f, 8.0f);
            }
        }
        else // Continuous audio / time signals: subtle, non-distracting slow flow
        {
            double speedFactor = conn.isFeedbackCycle ? 0.0006 : 0.00025;
            float pulsePos = static_cast<float>(std::fmod(millis * speedFactor, 1.0));
            auto dot = cablePath.getPointAlongPath(pulsePos * cablePath.getLength());

            g.setColour(typeColor.withAlpha(0.35f));
            g.fillEllipse(dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
        }

        if (conn.isFeedbackCycle)
        {
            auto midPt = cablePath.getPointAlongPath(0.5f * cablePath.getLength());
            g.setColour(juce::Colour::fromRGB(0xff, 0x44, 0x44));
            g.setFont(10.0f);
            g.drawText("1-blk z⁻¹", juce::Rectangle<float>(midPt.x - 25, midPt.y - 12, 50, 14), juce::Justification::centred, false);
        }
    }

    // Active drag cable
    if (draggingPortNodeId != -1)
    {
        auto node = currGraph.getNode(draggingPortNodeId);
        if (node)
        {
            auto p1 = getPortPos(*node, isDraggingFromOutlet, draggingPortIdx);
            juce::Path dragCable;
            dragCable.startNewSubPath(p1);
            dragCable.cubicTo(p1.x, p1.y + (isDraggingFromOutlet ? 40.0f : -40.0f),
                              dragCurrentPos.x, dragCurrentPos.y + (isDraggingFromOutlet ? -40.0f : 40.0f),
                              dragCurrentPos.x, dragCurrentPos.y);

            PortDataType dragType = PortDataType::Audio;
            if (isDraggingFromOutlet && draggingPortIdx < static_cast<int>(node->getOutlets().size()))
                dragType = node->getOutlets()[static_cast<size_t>(draggingPortIdx)].dataType;
            else if (!isDraggingFromOutlet && draggingPortIdx < static_cast<int>(node->getInlets().size()))
                dragType = node->getInlets()[static_cast<size_t>(draggingPortIdx)].dataType;

            juce::Colour dragColor = (dragType == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                                    ((dragType == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);

            g.setColour(dragColor);
            g.strokePath(dragCable, juce::PathStrokeType(2.5f));
        }
    }

    // Render Nodes
    for (const auto& node : currGraph.getNodes())
    {
        auto b = getNodeBounds(*node);
        bool isSelected = isNodeSelected(node->getId());

        std::string sym = node->getSymbol();
        std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

        // ---------------------------------------------------------------------
        // Custom Distinct GUI Objects
        // ---------------------------------------------------------------------
        if (sym == "msg" || sym == "message")
        {
            // Pure Data Classic Flag Notch Box (Trapezoid right edge cut)
            juce::Path msgPath;
            float notch = 12.0f;
            msgPath.startNewSubPath(b.getX(), b.getY());
            msgPath.lineTo(b.getRight() - notch, b.getY());
            msgPath.lineTo(b.getRight(), b.getY() + notch);
            msgPath.lineTo(b.getRight(), b.getBottom());
            msgPath.lineTo(b.getX(), b.getBottom());
            msgPath.closeSubPath();

            g.setColour(isSelected ? CarbonGoldLookAndFeel::slatePanel.brighter(0.25f) : juce::Colour::fromRGB(0x16, 0x1c, 0x28));
            g.fillPath(msgPath);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::goldAccent.withAlpha(0.85f));
            g.strokePath(msgPath, juce::PathStrokeType(isSelected ? 2.5f : 1.5f));

            g.setColour(CarbonGoldLookAndFeel::goldAccent);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(node->getLabel(), b.reduced(8.0f, 4.0f), juce::Justification::centredLeft, true);
        }
        else if (sym == "bang" || sym == "bng")
        {
            // Bang Circle Target Box
            auto bNode = std::dynamic_pointer_cast<BangNode>(node);
            bool flashing = bNode && bNode->isFlashing();

            g.setColour(flashing ? CarbonGoldLookAndFeel::goldAccent.withAlpha(0.35f) : CarbonGoldLookAndFeel::slatePanel);
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : (flashing ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan));
            g.drawRoundedRectangle(b, 4.0f, 1.5f);

            auto centerCircle = b.reduced(b.getWidth() * 0.22f, b.getHeight() * 0.22f);
            g.setColour(flashing ? juce::Colours::white : CarbonGoldLookAndFeel::goldAccent);
            g.drawEllipse(centerCircle, 2.0f);
            if (flashing) {
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.fillEllipse(centerCircle.reduced(3.0f));
            }
        }
        else if (sym == "toggle" || sym == "tgl")
        {
            // Toggle [X] / [ ] Box
            auto tNode = std::dynamic_pointer_cast<ToggleNode>(node);
            bool state = tNode ? tNode->getState() : false;

            g.setColour(state ? juce::Colour::fromRGB(0x1a, 0x22, 0x18) : CarbonGoldLookAndFeel::slatePanel);
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : (state ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::slatePanel.brighter(0.4f)));
            g.drawRoundedRectangle(b, 4.0f, 1.5f);

            g.setColour(state ? CarbonGoldLookAndFeel::goldAccent : juce::Colours::grey);
            g.setFont(juce::Font(14.0f, juce::Font::bold));
            g.drawText(state ? "[X]" : "[  ]", b, juce::Justification::centred, false);
        }
        else if (sym == "number" || sym == "num")
        {
            // Slanted Top-Right Corner Notch Box (/)
            juce::Path numPath;
            float notch = 10.0f;
            numPath.startNewSubPath(b.getX(), b.getY());
            numPath.lineTo(b.getRight() - notch, b.getY());
            numPath.lineTo(b.getRight(), b.getY() + notch);
            numPath.lineTo(b.getRight(), b.getBottom());
            numPath.lineTo(b.getX(), b.getBottom());
            numPath.closeSubPath();

            g.setColour(isSelected ? CarbonGoldLookAndFeel::slatePanel.brighter(0.2f) : juce::Colour::fromRGB(0x0a, 0x14, 0x22));
            g.fillPath(numPath);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);
            g.strokePath(numPath, juce::PathStrokeType(isSelected ? 2.5f : 1.5f));

            g.setColour(CarbonGoldLookAndFeel::cyberCyan);
            g.setFont(juce::Font(12.5f, juce::Font::bold));
            g.drawText(node->getLabel(), b.reduced(8.0f, 4.0f), juce::Justification::centredLeft, true);
        }
        else if (sym == "symbol" || sym == "sym")
        {
            // Symbol Text Box
            g.setColour(isSelected ? CarbonGoldLookAndFeel::slatePanel.brighter(0.2f) : juce::Colour::fromRGB(0x0e, 0x16, 0x24));
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan.withAlpha(0.8f));
            g.drawRoundedRectangle(b, 4.0f, 1.5f);

            g.setColour(CarbonGoldLookAndFeel::cyberCyan);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(node->getLabel(), b.reduced(8.0f, 4.0f), juce::Justification::centredLeft, true);
        }
        else if (sym == "radio" || sym == "hradio" || sym == "vradio")
        {
            // Radio Buttons Strip
            auto rNode = std::dynamic_pointer_cast<RadioNode>(node);
            int opts = rNode ? rNode->getNumOptions() : 4;
            int sel = rNode ? rNode->getSelectedIndex() : 0;

            g.setColour(CarbonGoldLookAndFeel::slatePanel);
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::slatePanel.brighter(0.4f));
            g.drawRoundedRectangle(b, 4.0f, 1.5f);

            float btnW = b.getWidth() / static_cast<float>(opts);
            for (int i = 0; i < opts; ++i) {
                auto rBox = juce::Rectangle<float>(b.getX() + i * btnW, b.getY(), btnW, b.getHeight()).reduced(5.0f);
                g.setColour(i == sel ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::slatePanel.brighter(0.4f));
                g.drawEllipse(rBox, 1.5f);
                if (i == sel) {
                    g.fillEllipse(rBox.reduced(3.0f));
                }
            }
        }
        else if (sym == "display" || sym == "disp" || sym == "print")
        {
            // Recessed Terminal Display Screen Box
            auto dNode = std::dynamic_pointer_cast<DisplayNode>(node);
            std::string dispStr = dNode ? dNode->getDisplayText() : "---";
            std::string tag = dNode && !dNode->getCustomTag().empty() ? dNode->getCustomTag() : (sym == "print" ? "print" : "disp");

            g.setColour(juce::Colour::fromRGB(0x06, 0x0a, 0x12));
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan.withAlpha(0.6f));
            g.drawRoundedRectangle(b, 4.0f, 1.5f);

            g.setColour(CarbonGoldLookAndFeel::cyberCyan);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(tag + ": " + dispStr, b.reduced(8.0f, 4.0f), juce::Justification::centredLeft, true);
        }
        else if (isControlLogicSymbol(sym))
        {
            // Clean Compact Pure Data Style Control / Logic Object Card
            g.setColour(isSelected ? CarbonGoldLookAndFeel::slatePanel.brighter(0.2f) : CarbonGoldLookAndFeel::slatePanel);
            g.fillRoundedRectangle(b, 4.0f);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::slatePanel.brighter(0.35f));
            g.drawRoundedRectangle(b, 4.0f, isSelected ? 2.0f : 1.0f);

            g.setColour(CarbonGoldLookAndFeel::goldAccent);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(node->getLabel(), b.reduced(8.0f, 2.0f), juce::Justification::centredLeft, true);
        }
        else if (isSequencerSymbol(sym))
        {
            // Dedicated Relativistic Sequencer Card (Mini-Notation & Progress Strip)
            g.setColour(isSelected ? CarbonGoldLookAndFeel::slatePanel.brighter(0.2f) : CarbonGoldLookAndFeel::slatePanel);
            g.fillRoundedRectangle(b, 5.0f);
            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::goldAccent.withAlpha(0.6f));
            g.drawRoundedRectangle(b, 5.0f, isSelected ? 2.0f : 1.0f);

            auto headerRect = b.removeFromTop(20.0f);
            g.setColour(CarbonGoldLookAndFeel::goldAccent);
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.drawText("♩ " + node->getSymbol(), headerRect.reduced(6.0f, 0.0f), juce::Justification::centredLeft, true);

            // Body Strip
            auto bodyRect = b.reduced(4.0f, 3.0f);
            g.setColour(juce::Colour::fromRGB(0x0a, 0x0e, 0x18));
            g.fillRoundedRectangle(bodyRect, 3.0f);
            g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.2f));
            g.drawRoundedRectangle(bodyRect, 3.0f, 1.0f);

            if (auto tidalNode = std::dynamic_pointer_cast<TidalSeqNode>(node))
            {
                const auto& events = tidalNode->getScheduledEvents();
                double phase = tidalNode->getCyclePhase();

                if (!events.empty())
                {
                    // Find max channels for polyphonic vertical stacking
                    int maxCh = 1;
                    for (const auto& ev : events) maxCh = std::max(maxCh, ev.channel + 1);

                    float laneH = bodyRect.getHeight() / static_cast<float>(maxCh);

                    for (const auto& ev : events)
                    {
                        float bx = bodyRect.getX() + static_cast<float>(ev.startCycle) * bodyRect.getWidth();
                        float bw = std::max(2.0f, static_cast<float>(ev.endCycle - ev.startCycle) * bodyRect.getWidth() - 1.0f);
                        float by = bodyRect.getY() + ev.channel * laneH;
                        auto blockRect = juce::Rectangle<float>(bx, by + 1.0f, bw, laneH - 2.0f);

                        bool isCurrentlyPlaying = (phase >= ev.startCycle && phase < ev.endCycle);

                        if (ev.isRest)
                        {
                            g.setColour(juce::Colour::fromRGB(0x12, 0x18, 0x24));
                            g.fillRoundedRectangle(blockRect, 2.0f);
                            g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.1f));
                            g.drawRoundedRectangle(blockRect, 2.0f, 0.8f);
                        }
                        else
                        {
                            juce::Colour blockCol = (ev.channel == 0) ? CarbonGoldLookAndFeel::royalViolet.withAlpha(0.7f)
                                                                      : CarbonGoldLookAndFeel::cyberCyan.withAlpha(0.6f);
                            if (isCurrentlyPlaying) blockCol = blockCol.brighter(0.4f);

                            g.setColour(blockCol);
                            g.fillRoundedRectangle(blockRect, 2.0f);

                            // Active Playing White Border (Strudel style)
                            if (isCurrentlyPlaying)
                            {
                                g.setColour(juce::Colours::white);
                                g.drawRoundedRectangle(blockRect, 2.0f, 2.0f);
                            }
                            else
                            {
                                g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.4f));
                                g.drawRoundedRectangle(blockRect, 2.0f, 1.0f);
                            }

                            // Pitch Label
                            if (blockRect.getWidth() > 14.0f)
                            {
                                g.setColour(isCurrentlyPlaying ? juce::Colours::white : CarbonGoldLookAndFeel::goldAccent);
                                g.setFont(juce::Font(9.0f, juce::Font::bold));
                                std::string lbl = std::to_string(ev.pitch);
                                if (ev.pitch == 36) lbl = "bd";
                                else if (ev.pitch == 38 || ev.pitch == 40) lbl = "sn";
                                else if (ev.pitch == 42) lbl = "hh";
                                g.drawText(lbl, blockRect, juce::Justification::centred, true);
                            }
                        }
                    }
                }
                else
                {
                    // Fallback to pattern string
                    g.setColour(CarbonGoldLookAndFeel::cyberCyan);
                    g.setFont(juce::Font(10.5f, juce::Font::bold));
                    g.drawText(tidalNode->getPatternString(), bodyRect.reduced(4.0f, 0.0f), juce::Justification::centredLeft, true);
                }

                // Edit pencil button indicator in top-right
                auto editBtnR = headerRect.removeFromRight(20.0f).reduced(2.0f);
                g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.7f));
                g.drawText("✏", editBtnR, juce::Justification::centred, false);
            }
            else if (auto euclidNode = std::dynamic_pointer_cast<EuclidSequencerNode>(node))
            {
                // Draw Euclidean step dots [● ○ ● ● ○ ●]
                const auto& pat = euclidNode->getPattern();
                int n = static_cast<int>(pat.size());
                if (n > 0)
                {
                    float dotSpacing = bodyRect.getWidth() / static_cast<float>(n);
                    float dotR = std::min(4.0f, dotSpacing * 0.35f);
                    float midY = bodyRect.getCentreY();

                    for (int i = 0; i < n; ++i)
                    {
                        float dx = bodyRect.getX() + (i + 0.5f) * dotSpacing;
                        if (pat[static_cast<size_t>(i)])
                        {
                            g.setColour(CarbonGoldLookAndFeel::goldAccent);
                            g.fillEllipse(dx - dotR, midY - dotR, dotR * 2.0f, dotR * 2.0f);
                        }
                        else
                        {
                            g.setColour(juce::Colours::grey.withAlpha(0.5f));
                            g.drawEllipse(dx - dotR, midY - dotR, dotR * 2.0f, dotR * 2.0f, 1.0f);
                        }
                    }
                }
            }
            else
            {
                // Generic sequencer label / step info
                g.setColour(CarbonGoldLookAndFeel::cyberCyan);
                g.setFont(juce::Font(10.5f, juce::Font::bold));
                g.drawText(node->getLabel(), bodyRect.reduced(6.0f, 2.0f), juce::Justification::centredLeft, true);
            }
        }
        else
        {
            // Standard Audio Signal or Relativistic Time Sculptor Processing Card
            bool isTimeSculptor = isTimeSculptorSymbol(sym);

            g.setColour(isSelected ? CarbonGoldLookAndFeel::slatePanel.brighter(0.2f) : CarbonGoldLookAndFeel::slatePanel);
            g.fillRoundedRectangle(b, 5.0f);

            g.setColour(isSelected ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::slatePanel.brighter(0.4f));
            g.drawRoundedRectangle(b, 5.0f, isSelected ? 2.5f : 1.0f);

            // Header Title Label with Wrapping Support
            auto headerRect = b.removeFromTop(22.0f);

            // Realtime Display Toggle Button [👁]
            auto toggleBtnRect = headerRect.removeFromRight(22.0f).reduced(2.0f);

            // Scope Mode Toggle Button (Only for Time Sculptors or Multi-Mode nodes)
            juce::Rectangle<float> modeBtnRect;
            if (isTimeSculptor)
            {
                modeBtnRect = headerRect.removeFromRight(46.0f).reduced(2.0f);
            }

            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawFittedText(node->getLabel(), headerRect.reduced(6.0f, 0.0f).toNearestInt(), juce::Justification::left, 2, 0.9f);

            g.setColour(node->showRealtimeDisplay ? CarbonGoldLookAndFeel::goldAccent : juce::Colours::grey);
            g.drawRoundedRectangle(toggleBtnRect, 3.0f, 1.0f);
            g.setFont(10.0f);
            g.drawText("👁", toggleBtnRect, juce::Justification::centred, false);

            if (isTimeSculptor)
            {
                node->displayType = RelativisticNode::ScopeDisplayType::TimeFrame;
                g.setColour(CarbonGoldLookAndFeel::royalViolet);
                g.drawRoundedRectangle(modeBtnRect, 3.0f, 1.0f);
                g.setFont(9.0f);
                juce::String modeStr;
                if (node->timeVarMode == RelativisticNode::TimeScopeVariable::SpeedGamma) modeStr = "Speed";
                else if (node->timeVarMode == RelativisticNode::TimeScopeVariable::OffsetTau) modeStr = "Offset";
                else if (node->timeVarMode == RelativisticNode::TimeScopeVariable::CouplingC) modeStr = "Flex";
                else modeStr = "Multi";
                g.drawText(modeStr, modeBtnRect, juce::Justification::centred, false);
            }

            // Realtime Scope & Value Display Area
            if (node->showRealtimeDisplay && b.getHeight() > 30.0f)
            {
                auto scopeBox = b.reduced(4.0f, 4.0f);
                g.setColour(juce::Colour::fromRGB(0x10, 0x12, 0x18));
                g.fillRoundedRectangle(scopeBox, 3.0f);
                g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter(0.3f));
                g.drawRoundedRectangle(scopeBox, 3.0f, 1.0f);

            // Fetch live TimePolyFrame data across all outlets (first) or inlets
            const TimePolyFrame* frame = nullptr;
            for (size_t i = 0; i < node->getOutlets().size(); ++i)
            {
                if (node->getOutlets()[i].dataType == PortDataType::Time)
                {
                    frame = &node->getOutletTimeFrame(static_cast<int>(i));
                    break;
                }
            }
            if (!frame)
            {
                for (size_t i = 0; i < node->getInlets().size(); ++i)
                {
                    if (node->getInlets()[i].dataType == PortDataType::Time)
                    {
                        frame = &node->getInletTimeFrame(static_cast<int>(i));
                        break;
                    }
                }
            }

            double currentGamma = frame ? frame->masterGamma : 1.0;
            double currentTau = (frame && !frame->streams.empty()) ? frame->streams[0].tau : 0.0;

            // Sample latest instantaneous audio-rate telemetry from timeScopeBuffers
            if (!node->timeScopeBuffer.empty())
            {
                size_t len = node->timeScopeBuffer.size();
                size_t lastIdx = (node->timeScopeWriteIdx + len - 1) % len;
                float lastVal = node->timeScopeBuffer[lastIdx];
                if (std::abs(lastVal) > 0.0001f) currentGamma = static_cast<double>(lastVal);
            }
            if (!node->timeTauScopeBuffer.empty())
            {
                size_t len = node->timeTauScopeBuffer.size();
                size_t lastIdx = (node->timeTauScopeWriteIdx + len - 1) % len;
                currentTau = static_cast<double>(node->timeTauScopeBuffer[lastIdx]);
            }

            double tHiRes = juce::Time::getMillisecondCounterHiRes() * 0.001;

            if (auto outNode = std::dynamic_pointer_cast<OutNode>(node))
            {
                float rmsL = outNode->getRmsL();
                float rmsR = outNode->getRmsR();
                const auto& waveBuf = outNode->getWaveformBuffer();
                size_t writeIdx = outNode->getWaveformWritePos();

                // Draw Waveform Oscilloscope inside scopeBox
                auto waveArea = scopeBox.withTrimmedRight(28.0f);
                g.setColour(juce::Colour::fromRGB(0x0a, 0x0c, 0x10));
                g.fillRect(waveArea);
                g.setColour(CarbonGoldLookAndFeel::cyberCyan.withAlpha(0.25f));
                g.drawHorizontalLine(static_cast<int>(waveArea.getCentreY()), waveArea.getX(), waveArea.getRight());

                if (!waveBuf.empty())
                {
                    juce::Path wavePath;
                    float midY = waveArea.getCentreY();
                    float h = waveArea.getHeight() * 0.45f;
                    float w = waveArea.getWidth();
                    size_t len = waveBuf.size();
                    size_t readStart = (writeIdx + len - 128) % len;

                    for (int i = 0; i < 128; ++i)
                    {
                        float val = waveBuf[(readStart + i) % len];
                        float px = waveArea.getX() + (i / 128.0f) * w;
                        float py = midY - val * h;
                        if (i == 0) wavePath.startNewSubPath(px, py);
                        else wavePath.lineTo(px, py);
                    }
                    g.setColour(CarbonGoldLookAndFeel::goldAccent);
                    g.strokePath(wavePath, juce::PathStrokeType(1.5f));
                }

                // Draw Dual L/R RMS & Peak Meters on the right side
                auto meterArea = scopeBox.withLeft(waveArea.getRight() + 2.0f);
                auto lMeter = meterArea.removeFromLeft(11.0f).reduced(1.0f, 2.0f);
                auto rMeter = meterArea.removeFromLeft(11.0f).reduced(1.0f, 2.0f);

                float peakL = outNode->getPeakL();
                float peakR = outNode->getPeakR();
                bool clipL = outNode->isClippingL() || (peakL >= 1.0f);
                bool clipR = outNode->isClippingR() || (peakR >= 1.0f);

                auto drawMeter = [&](juce::Rectangle<float> rect, float level, float peak, bool isClipping) {
                    // Split top 4px for Clip LED
                    auto ledRect = rect.removeFromTop(4.0f);
                    auto barRect = rect.withTrimmedTop(2.0f);

                    // 1. Render Clip LED
                    if (isClipping)
                    {
                        g.setColour(juce::Colour::fromRGB(0xff, 0x22, 0x44)); // Bright Red Clip Glow
                        g.fillRoundedRectangle(ledRect, 1.0f);
                        g.setColour(juce::Colours::white.withAlpha(0.8f));
                        g.drawRoundedRectangle(ledRect, 1.0f, 0.8f);
                    }
                    else
                    {
                        g.setColour(juce::Colour::fromRGB(0x24, 0x08, 0x0c)); // Dark Maroon Idle LED
                        g.fillRoundedRectangle(ledRect, 1.0f);
                        g.setColour(juce::Colour::fromRGB(0x40, 0x12, 0x18));
                        g.drawRoundedRectangle(ledRect, 1.0f, 0.5f);
                    }

                    // 2. Render Main Level Bar
                    g.setColour(juce::Colours::black);
                    g.fillRect(barRect);
                    float fillH = barRect.getHeight() * std::clamp(level * 2.0f, 0.0f, 1.0f);
                    auto fillRect = barRect.withHeight(fillH).withY(barRect.getBottom() - fillH);
                    juce::ColourGradient grad(juce::Colours::lime, fillRect.getX(), fillRect.getBottom(),
                                                (level > 0.8f ? juce::Colours::red : CarbonGoldLookAndFeel::goldAccent), fillRect.getX(), fillRect.getY(), false);
                    g.setGradientFill(grad);
                    g.fillRect(fillRect);

                    // Peak line indicator
                    float peakY = barRect.getBottom() - barRect.getHeight() * std::clamp(peak * 2.0f, 0.0f, 1.0f);
                    g.setColour(isClipping ? juce::Colours::red : juce::Colours::white);
                    g.drawHorizontalLine(static_cast<int>(peakY), barRect.getX(), barRect.getRight());

                    g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter());
                    g.drawRect(barRect, 1.0f);
                };

                drawMeter(lMeter, rmsL, peakL, clipL);
                drawMeter(rMeter, rmsR, peakR, clipR);

                // RMS & Master Gain Overlay Text
                char valBuf[64];
                std::snprintf(valBuf, sizeof(valBuf), "L:%.2f R:%.2f", rmsL, rmsR);
                g.setColour(clipL || clipR ? juce::Colour::fromRGB(0xff, 0x44, 0x55) : CarbonGoldLookAndFeel::cyberCyan);
                g.setFont(9.0f);
                g.drawText(valBuf, waveArea.reduced(2.0f, 1.0f), juce::Justification::topRight, false);
            }
            else if (auto meterNode = std::dynamic_pointer_cast<MeterNode>(node))
            {
                g.setColour(juce::Colour::fromRGB(0x0a, 0x0c, 0x10));
                g.fillRect(scopeBox);

                float levelDb = meterNode->getMeasuredLevelDb();
                float peakDb = meterNode->getPeakLevelDb();
                std::string modeName = meterNode->getMeterModeName();

                // Map dB [-60.0 to +6.0] -> [0.0 to 1.0] norm level
                float normLevel = std::clamp((levelDb + 60.0f) / 66.0f, 0.0f, 1.0f);
                float normPeak = std::clamp((peakDb + 60.0f) / 66.0f, 0.0f, 1.0f);

                auto meterBar = scopeBox.reduced(6.0f, 8.0f);
                float fillW = meterBar.getWidth() * normLevel;
                auto fillRect = meterBar.withWidth(fillW);

                juce::ColourGradient grad(CarbonGoldLookAndFeel::cyberCyan, meterBar.getX(), meterBar.getY(),
                                          (levelDb > 0.0f ? juce::Colours::red : CarbonGoldLookAndFeel::goldAccent), meterBar.getRight(), meterBar.getY(), false);
                g.setGradientFill(grad);
                g.fillRect(fillRect);

                // Peak indicator line
                float peakX = meterBar.getX() + meterBar.getWidth() * normPeak;
                g.setColour(juce::Colours::white);
                g.drawVerticalLine(static_cast<int>(peakX), meterBar.getY(), meterBar.getBottom());

                g.setColour(CarbonGoldLookAndFeel::slatePanel.brighter());
                g.drawRect(meterBar, 1.0f);

                // Mode & dB Level Overlay Text
                juce::String tagText = "[" + juce::String(modeName) + "] " +
                    (levelDb <= -99.0f ? "-\u221e dB" : juce::String(levelDb, 1) + (meterNode->getMeterMode() == MeterNode::MeterMode::LUFS ? " LUFS" : " dB"));
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.setFont(juce::Font(10.0f, juce::Font::bold));
                g.drawText(tagText, scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
            }
            else if (auto specNode = std::dynamic_pointer_cast<SpectrogramNode>(node))
            {
                g.setColour(juce::Colour::fromRGB(0x06, 0x09, 0x12));
                g.fillRect(scopeBox);

                const auto& grid = specNode->getSpectrogramGrid();
                int writeSlice = specNode->getGridWriteIndex();
                int numBins = SpectrogramNode::numBins;
                int histLen = SpectrogramNode::historyLength;

                if (!grid.empty())
                {
                    int w = static_cast<int>(scopeBox.getWidth());
                    int h = static_cast<int>(scopeBox.getHeight());
                    if (w > 0 && h > 0)
                    {
                        juce::Image specImg(juce::Image::RGB, histLen, numBins, true);
                        for (int t = 0; t < histLen; ++t)
                        {
                            int sliceIdx = (writeSlice + t) % histLen;
                            size_t rowOffset = static_cast<size_t>(sliceIdx) * static_cast<size_t>(numBins);

                            for (int b = 0; b < numBins; ++b)
                            {
                                float val = grid[rowOffset + static_cast<size_t>(b)];
                                int py = numBins - 1 - b; // Nyquist freq at top, 0 Hz at bottom

                                // Carbon & Gold Colorful Spectrogram Palette (Non-distracting)
                                juce::Colour col;
                                if (val < 0.05f) col = juce::Colour::fromRGB(0x06, 0x09, 0x12);
                                else if (val < 0.25f) col = juce::Colour::fromRGB(0x28, 0x12, 0x48); // Royal Violet low
                                else if (val < 0.55f) col = juce::Colour::fromRGB(0x00, 0x88, 0xcc); // Cyber Cyan mid
                                else if (val < 0.85f) col = juce::Colour::fromRGB(0xd4, 0x98, 0x00); // Warm Gold high
                                else col = juce::Colour::fromRGB(0xff, 0xf0, 0xd0);                 // Gold-White peak

                                specImg.setPixelAt(t, py, col);
                            }
                        }

                        g.drawImage(specImg, scopeBox, juce::RectanglePlacement::stretchToFit);
                    }
                }

                // Nyquist and Spectrum Overlay Label
                double nyquist = specNode->getNyquistFreq();
                juce::String nyquistStr = "0Hz - " + juce::String(static_cast<int>(nyquist / 1000.0)) + "kHz";
                g.setColour(CarbonGoldLookAndFeel::cyberCyan.withAlpha(0.9f));
                g.setFont(9.0f);
                g.drawText(nyquistStr, scopeBox.reduced(3.0f, 2.0f), juce::Justification::topRight, false);
            }
            else if (node->scopeMode == RelativisticNode::ScopeRenderMode::Waveform2D)
            {
                // 2D Waveform Scope (Audio Output or Proper Time Telemetry Plot)
                const auto& scopeBuf = (node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform)
                                       ? node->audioScopeBuffer : node->timeScopeBuffer;
                size_t writeIdx = (node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform)
                                  ? node->audioScopeWriteIdx : node->timeScopeWriteIdx;

                juce::Path wavePath;
                float midY = scopeBox.getCentreY();
                float h = scopeBox.getHeight() * 0.42f;
                float w = scopeBox.getWidth();

                if (!scopeBuf.empty())
                {
                    size_t len = scopeBuf.size();
                    size_t readStart = (writeIdx + len - 128) % len;

                    if (node->displayType == RelativisticNode::ScopeDisplayType::AudioWaveform)
                    {
                        // Direct Audio Waveform Oscilloscope
                        for (int i = 0; i < 128; ++i)
                        {
                            float sampleVal = scopeBuf[(readStart + i) % len];
                            float px = scopeBox.getX() + (i / 128.0f) * w;
                            float py = midY - std::clamp(sampleVal, -1.0f, 1.0f) * h;
                            if (i == 0) wavePath.startNewSubPath(px, py);
                            else wavePath.lineTo(px, py);
                        }
                        g.setColour(CarbonGoldLookAndFeel::cyberCyan);
                        g.strokePath(wavePath, juce::PathStrokeType(1.4f));

                        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.85f));
                        g.setFont(9.0f);
                        g.drawText("Audio Wave", scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
                    }
                    else
                    {
                        // Proper Time Telemetry Plot (Relative to baseline / mean)
                        float meanVal = 0.0f;
                        float maxDev = 0.001f;
                        for (int i = 0; i < 128; ++i)
                        {
                            meanVal += scopeBuf[(readStart + i) % len];
                        }
                        meanVal /= 128.0f;

                        for (int i = 0; i < 128; ++i)
                        {
                            maxDev = std::max(maxDev, std::abs(scopeBuf[(readStart + i) % len] - meanVal));
                        }
                        float normScale = (maxDev > 0.0001f) ? (0.85f / maxDev) : 1.0f;

                        for (int i = 0; i < 128; ++i)
                        {
                            float sampleVal = scopeBuf[(readStart + i) % len];
                            float normVal = (sampleVal - meanVal) * normScale;
                            float px = scopeBox.getX() + (i / 128.0f) * w;
                            float py = midY - normVal * h;
                            if (i == 0) wavePath.startNewSubPath(px, py);
                            else wavePath.lineTo(px, py);
                        }
                        g.setColour(CarbonGoldLookAndFeel::royalViolet);
                        g.strokePath(wavePath, juce::PathStrokeType(1.4f));

                        if (node->timeVarMode == RelativisticNode::TimeScopeVariable::MultiTime)
                        {
                            juce::Path tauPath;
                            const auto& tauBuf = node->timeTauScopeBuffer;
                            size_t tauWriteIdx = node->timeTauScopeWriteIdx;
                            size_t tauLen = tauBuf.size();

                            if (tauLen >= 128)
                            {
                                int tauReadStart = (static_cast<int>(tauWriteIdx) + static_cast<int>(tauLen) - 128) % static_cast<int>(tauLen);
                                float tauMean = 0.0f;
                                for (int i = 0; i < 128; ++i) tauMean += tauBuf[(tauReadStart + i) % tauLen];
                                tauMean /= 128.0f;

                                float tauMaxDev = 0.001f;
                                for (int i = 0; i < 128; ++i) tauMaxDev = std::max(tauMaxDev, std::abs(tauBuf[(tauReadStart + i) % tauLen] - tauMean));
                                float tauScale = (tauMaxDev > 0.0001f) ? (0.85f / tauMaxDev) : 1.0f;

                                for (int i = 0; i < 128; ++i)
                                {
                                    float sampleVal = tauBuf[(tauReadStart + i) % tauLen];
                                    float normVal = (sampleVal - tauMean) * tauScale;
                                    float px = scopeBox.getX() + (i / 128.0f) * w;
                                    float py = midY - normVal * h;
                                    if (i == 0) tauPath.startNewSubPath(px, py);
                                    else tauPath.lineTo(px, py);
                                }
                                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                                g.strokePath(tauPath, juce::PathStrokeType(1.4f));
                            }
                        }

                        juce::String valStr = "Speed: " + juce::String(currentGamma, 3) + "x  Offset: " + juce::String(currentTau, 2) + "s";
                        g.setColour(CarbonGoldLookAndFeel::goldAccent);
                        g.setFont(10.0f);
                        g.drawText(valStr, scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
                    }
                }
            }
            else if (node->scopeMode == RelativisticNode::ScopeRenderMode::ScopeXY)
            {
                // XY Lissajous Scope (Gamma vs Tau)
                g.setColour(CarbonGoldLookAndFeel::royalViolet.withAlpha(0.3f));
                g.drawHorizontalLine(static_cast<int>(scopeBox.getCentreY()), scopeBox.getX(), scopeBox.getRight());
                g.drawVerticalLine(static_cast<int>(scopeBox.getCentreX()), scopeBox.getY(), scopeBox.getBottom());

                juce::Path xyPath;
                float cx = scopeBox.getCentreX();
                float cy = scopeBox.getCentreY();
                float rx = scopeBox.getWidth() * 0.35f;
                float ry = scopeBox.getHeight() * 0.35f;

                for (int pt = 0; pt < 40; ++pt)
                {
                    double phase = (pt / 40.0) * 2.0 * 3.14159;
                    float x = cx + static_cast<float>(std::cos(phase * currentGamma)) * rx;
                    float y = cy + static_cast<float>(std::sin(phase + currentTau * 2.0)) * ry;
                    if (pt == 0) xyPath.startNewSubPath(x, y);
                    else xyPath.lineTo(x, y);
                }
                xyPath.closeSubPath();
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.strokePath(xyPath, juce::PathStrokeType(1.5f));

                juce::String valStr = "Speed: " + juce::String(currentGamma, 3) + "x  Offset: " + juce::String(currentTau, 2) + "s";
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.setFont(10.0f);
                g.drawText(valStr, scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
            }
            else if (node->scopeMode == RelativisticNode::ScopeRenderMode::Scope3D)
            {
                // 3D Isometric Time-Space Projection
                float cx = scopeBox.getCentreX();
                float cy = scopeBox.getCentreY();
                float size = std::min(scopeBox.getWidth(), scopeBox.getHeight()) * 0.35f;
                double rotAngle = tHiRes * 0.8;

                auto project3D = [&](float x3d, float y3d, float z3d) -> juce::Point<float> {
                    float rx = x3d * static_cast<float>(std::cos(rotAngle)) - z3d * static_cast<float>(std::sin(rotAngle));
                    float rz = x3d * static_cast<float>(std::sin(rotAngle)) + z3d * static_cast<float>(std::cos(rotAngle));
                    float isoX = cx + (rx - y3d) * 0.707f;
                    float isoY = cy + (rx + y3d) * 0.408f - rz * 0.5f;
                    return { isoX, isoY };
                };

                // 3D Wireframe Cube
                g.setColour(CarbonGoldLookAndFeel::royalViolet.withAlpha(0.5f));
                auto p000 = project3D(-size, -size, -size);
                auto p100 = project3D( size, -size, -size);
                auto p110 = project3D( size,  size, -size);
                auto p010 = project3D(-size,  size, -size);
                auto p001 = project3D(-size, -size,  size);
                auto p101 = project3D( size, -size,  size);
                auto p111 = project3D( size,  size,  size);
                auto p011 = project3D(-size,  size,  size);

                g.drawLine(p000.x, p000.y, p100.x, p100.y, 1.0f);
                g.drawLine(p100.x, p100.y, p110.x, p110.y, 1.0f);
                g.drawLine(p110.x, p110.y, p010.x, p010.y, 1.0f);
                g.drawLine(p010.x, p010.y, p000.x, p000.y, 1.0f);

                g.drawLine(p001.x, p001.y, p101.x, p101.y, 1.0f);
                g.drawLine(p101.x, p101.y, p111.x, p111.y, 1.0f);
                g.drawLine(p111.x, p111.y, p011.x, p011.y, 1.0f);
                g.drawLine(p011.x, p011.y, p001.x, p001.y, 1.0f);

                g.drawLine(p000.x, p000.y, p001.x, p001.y, 1.0f);
                g.drawLine(p100.x, p100.y, p101.x, p101.y, 1.0f);
                g.drawLine(p110.x, p110.y, p111.x, p111.y, 1.0f);
                g.drawLine(p010.x, p010.y, p011.x, p011.y, 1.0f);

                // 3D Relativistic Trajectory Line
                juce::Path path3D;
                for (int i = 0; i < 30; ++i)
                {
                    float tStep = (i / 30.0f) * 2.0f - 1.0f;
                    float xVal = tStep * size;
                    float yVal = static_cast<float>(std::sin(tStep * 3.14159 * currentGamma)) * size;
                    float zVal = static_cast<float>(std::cos(tStep * 3.14159 + currentTau)) * size;
                    auto pt = project3D(xVal, yVal, zVal);
                    if (i == 0) path3D.startNewSubPath(pt);
                    else path3D.lineTo(pt);
                }
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.strokePath(path3D, juce::PathStrokeType(1.5f));

                juce::String valStr = "Speed: " + juce::String(currentGamma, 3) + "x  Offset: " + juce::String(currentTau, 2) + "s";
                g.setColour(CarbonGoldLookAndFeel::goldAccent);
                g.setFont(9.0f);
                g.drawText(valStr, scopeBox.reduced(4.0f, 2.0f), juce::Justification::topRight, false);
            }
        }

        if (node->getSymbol() == "patch~" || node->getSymbol() == "osc.patch~")
        {
            g.setColour(CarbonGoldLookAndFeel::royalViolet);
            g.drawText("[🔍 drill-down]", b.removeFromBottom(16.0f), juce::Justification::centred, true);
        }
        } // end if (!isControlGuiObject)

        // Bottom-Right Corner Resize Grip Handle for Selected Nodes
        if (isSelected)
        {
            auto gripRect = juce::Rectangle<float>(getNodeBounds(*node).getRight() - 12.0f, getNodeBounds(*node).getBottom() - 12.0f, 10.0f, 10.0f);
            g.setColour(CarbonGoldLookAndFeel::goldAccent);
            g.fillRoundedRectangle(gripRect, 2.0f);
            g.setColour(juce::Colours::black);
            g.drawLine(gripRect.getX() + 2, gripRect.getBottom() - 2, gripRect.getRight() - 2, gripRect.getY() + 2, 1.0f);
        }

        // Inlets
        for (int i = 0; i < static_cast<int>(node->getInlets().size()); ++i)
        {
            auto p = getPortPos(*node, false, i);
            auto type = node->getInlets()[static_cast<size_t>(i)].dataType;
            auto pColor = (type == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                         ((type == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);
            g.setColour(pColor);
            g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        }

        // Outlets
        for (int o = 0; o < static_cast<int>(node->getOutlets().size()); ++o)
        {
            auto p = getPortPos(*node, true, o);
            auto type = node->getOutlets()[static_cast<size_t>(o)].dataType;
            auto pColor = (type == PortDataType::Time) ? CarbonGoldLookAndFeel::royalViolet :
                         ((type == PortDataType::Message) ? CarbonGoldLookAndFeel::goldAccent : CarbonGoldLookAndFeel::cyberCyan);
            g.setColour(pColor);
            g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        }
    }

    // Paint Marquee / Lasso Selection Box
    if (isMarqueeSelecting && !marqueeRect.isEmpty())
    {
        g.setColour(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.15f));
        g.fillRect(marqueeRect);
        g.setColour(CarbonGoldLookAndFeel::goldAccent);
        g.drawRect(marqueeRect, 1.5f);
    }
}

void RelativisticCanvasComponent::resized()
{
    recenterButton.setBounds(getWidth() - 110, getHeight() - 35, 100, 24);
}

void RelativisticCanvasComponent::updateAutocompleteSuggestions()
{
    if (!isEditingObject)
    {
        suggestionListBox.setVisible(false);
        return;
    }

    juce::String text = objectEditor.getText().toLowerCase().trim();
    filteredObjects.clear();

    if (text.isEmpty())
    {
        suggestionListBox.setVisible(false);
        return;
    }

    auto searchTokens = juce::StringArray::fromTokens(text, " \t,", "");

    struct ScoredItem
    {
        AutocompleteItem item;
        int score = 0;
    };
    std::vector<ScoredItem> scored;

    for (const auto& item : allCatalogueObjects)
    {
        juce::String sym = juce::String(item.symbol).toLowerCase();
        juce::String desc = juce::String(item.description).toLowerCase();
        juce::String kw = juce::String(item.keywords).toLowerCase();

        bool allTokensMatch = true;
        int matchScore = 0;

        for (const auto& tok : searchTokens)
        {
            if (sym.startsWith(tok))
            {
                matchScore += 100;
            }
            else if (sym.contains(tok))
            {
                matchScore += 60;
            }
            else if (kw.contains(tok))
            {
                matchScore += 40;
            }
            else if (desc.contains(tok))
            {
                matchScore += 20;
            }
            else
            {
                allTokensMatch = false;
                break;
            }
        }

        if (allTokensMatch && matchScore > 0)
        {
            scored.push_back({ item, matchScore });
        }
    }

    std::stable_sort(scored.begin(), scored.end(), [](const ScoredItem& a, const ScoredItem& b) {
        return a.score > b.score;
    });

    for (const auto& s : scored)
    {
        filteredObjects.push_back(s.item);
    }

    if (!filteredObjects.empty())
    {
        int x = objectEditor.getX();
        int y = objectEditor.getBottom() + 2;
        int w = std::max(560, objectEditor.getWidth());
        int h = std::min(220, static_cast<int>(filteredObjects.size()) * 22 + 6);

        if (y + h > getHeight()) y = objectEditor.getY() - h - 2;

        suggestionListBox.setBounds(x, y, w, h);
        suggestionListBox.updateContent();
        suggestionListBox.setVisible(true);
        suggestionListBox.toFront(false);
    }
    else
    {
        suggestionListBox.setVisible(false);
    }
}

void RelativisticCanvasComponent::selectAutocompleteSuggestion(int row)
{
    if (row >= 0 && row < static_cast<int>(filteredObjects.size()))
    {
        objectEditor.setText(filteredObjects[static_cast<size_t>(row)].symbol);
        commitObjectCreation();
    }
}

int RelativisticCanvasComponent::AutocompleteModel::getNumRows()
{
    return static_cast<int>(canvas.filteredObjects.size());
}

void RelativisticCanvasComponent::AutocompleteModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(canvas.filteredObjects.size())) return;

    if (rowIsSelected) g.fillAll(CarbonGoldLookAndFeel::goldAccent.withAlpha(0.25f));
    else g.fillAll(juce::Colour::fromRGB(0x0e, 0x12, 0x1c));

    const auto& item = canvas.filteredObjects[static_cast<size_t>(rowNumber)];

    g.setColour(CarbonGoldLookAndFeel::goldAccent);
    g.setFont(juce::Font(11.5f, juce::Font::bold));
    int symW = 210;
    g.drawText(item.symbol, 6, 0, symW, height, juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);
    g.drawText(item.description, symW + 10, 0, width - (symW + 16), height, juce::Justification::centredLeft, true);
}

void RelativisticCanvasComponent::AutocompleteModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    canvas.selectAutocompleteSuggestion(row);
}

void RelativisticCanvasComponent::spawnObjectEditorAt(juce::Point<float> pos)
{
    lastMousePos = pos;
    isEditingObject = true;
    editingNodeId = -1;
    objectEditor.setText("");
    objectEditor.setBounds(static_cast<int>(pos.x), static_cast<int>(pos.y), 140, 30);
    objectEditor.setVisible(true);
    objectEditor.grabKeyboardFocus();
    updateAutocompleteSuggestions();
}

void RelativisticCanvasComponent::spawnObjectEditorForNode(int nodeId)
{
    auto node = getCurrentGraph().getNode(nodeId);
    if (!node) return;

    isEditingObject = true;
    editingNodeId = nodeId;
    lastMousePos = { node->xPos, node->yPos };

    juce::String editorText = node->getLabel();
    if (auto tidalNode = std::dynamic_pointer_cast<TidalSeqNode>(node))
    {
        editorText = "seq.tidal " + juce::String(tidalNode->getPatternString());
    }

    objectEditor.setText(editorText);
    auto b = getNodeBounds(*node);
    int edW = std::max(static_cast<int>(b.getWidth()), static_cast<int>(editorText.length() * 9 + 40));
    objectEditor.setBounds(static_cast<int>(b.getX()), static_cast<int>(b.getY()), edW, 30);
    objectEditor.setVisible(true);
    objectEditor.selectAll();
    objectEditor.grabKeyboardFocus();
    updateAutocompleteSuggestions();
}

void RelativisticCanvasComponent::commitObjectCreation()
{
    if (!isEditingObject) return;

    juce::String text = objectEditor.getText().trim();
    objectEditor.setVisible(false);
    suggestionListBox.setVisible(false);
    isEditingObject = false;

    if (text.isNotEmpty())
    {
        if (editingNodeId != -1)
        {
            // Re-editing existing node arguments in-place (PRESERVES ALL WIRE CONNECTIONS!)
            auto oldNode = getCurrentGraph().getNode(editingNodeId);
            if (oldNode)
            {
                if (auto tidalNode = std::dynamic_pointer_cast<TidalSeqNode>(oldNode))
                {
                    tidalNode->setPattern(text.toStdString());
                }
                else
                {
                    oldNode->setLabel(text.toStdString());
                    oldNode->receiveMessage(text.toStdString());
                }
                clearSelection();
                selectNode(oldNode->getId());
                if (onNodeSelected) onNodeSelected(oldNode);
            }
        }
        else
        {
            // Spawning new node!
            static int nextTempId = 100;
            auto newNode = RelativisticNodeFactory::createNode(nextTempId++, text.toStdString());
            if (newNode)
            {
                newNode->xPos = lastMousePos.x;
                newNode->yPos = lastMousePos.y;
                getCurrentGraph().addNode(newNode);
                clearSelection();
                selectNode(newNode->getId());
                if (onNodeSelected) onNodeSelected(newNode);
            }
        }
    }
    editingNodeId = -1;
    repaint();
}

void RelativisticCanvasComponent::cancelObjectCreation()
{
    objectEditor.setVisible(false);
    suggestionListBox.setVisible(false);
    isEditingObject = false;
    editingNodeId = -1;
    repaint();
}

void RelativisticCanvasComponent::selectAllNodes()
{
    clearSelection();
    for (const auto& node : getCurrentGraph().getNodes())
    {
        selectNode(node->getId());
    }
    repaint();
}

void RelativisticCanvasComponent::copySelectedNodes()
{
    clipboard.nodes.clear();
    clipboard.connections.clear();

    if (selectedNodeIds.empty()) return;

    for (int id : selectedNodeIds)
    {
        auto node = getCurrentGraph().getNode(id);
        if (!node) continue;

        ClipboardNodeData cNode;
        cNode.originalId = node->getId();
        cNode.symbol = node->getSymbol();
        cNode.label = node->getLabel();
        cNode.xPos = node->xPos;
        cNode.yPos = node->yPos;
        cNode.width = node->width;
        cNode.height = node->height;
        clipboard.nodes.push_back(cNode);
    }

    for (const auto& conn : getCurrentGraph().getConnections())
    {
        if (isNodeSelected(conn.sourceNodeId) && isNodeSelected(conn.destNodeId))
        {
            ClipboardConnectionData cConn;
            cConn.sourceNodeId = conn.sourceNodeId;
            cConn.sourcePortIndex = conn.sourcePortIndex;
            cConn.destNodeId = conn.destNodeId;
            cConn.destPortIndex = conn.destPortIndex;
            cConn.dataType = conn.dataType;
            clipboard.connections.push_back(cConn);
        }
    }
}

void RelativisticCanvasComponent::cutSelectedNodes()
{
    copySelectedNodes();
    deleteSelectedNodes();
}

void RelativisticCanvasComponent::pasteClipboardNodes()
{
    if (clipboard.isEmpty()) return;

    static int nextPasteId = 1000;
    std::unordered_map<int, int> oldToNewIdMap;

    clearSelection();

    // Paste nodes with +30px, +30px offset
    for (const auto& cNode : clipboard.nodes)
    {
        int newId = nextPasteId++;
        oldToNewIdMap[cNode.originalId] = newId;

        auto newNode = RelativisticNodeFactory::createNode(newId, cNode.label);
        if (!newNode) newNode = RelativisticNodeFactory::createNode(newId, cNode.symbol);

        if (newNode)
        {
            newNode->setLabel(cNode.label);
            newNode->xPos = cNode.xPos + 30.0f;
            newNode->yPos = cNode.yPos + 30.0f;
            newNode->width = cNode.width;
            newNode->height = cNode.height;
            getCurrentGraph().addNode(newNode);
            selectNode(newId);
        }
    }

    // Re-create internal connections between pasted nodes
    for (const auto& cConn : clipboard.connections)
    {
        auto srcIt = oldToNewIdMap.find(cConn.sourceNodeId);
        auto destIt = oldToNewIdMap.find(cConn.destNodeId);
        if (srcIt != oldToNewIdMap.end() && destIt != oldToNewIdMap.end())
        {
            getCurrentGraph().addConnection(srcIt->second, cConn.sourcePortIndex, destIt->second, cConn.destPortIndex);
        }
    }

    repaint();
}

void RelativisticCanvasComponent::duplicateSelectedNodes()
{
    copySelectedNodes();
    pasteClipboardNodes();
}

void RelativisticCanvasComponent::deleteSelectedNodes()
{
    if (selectedNodeIds.empty() && selectedConnectionId == -1) return;

    if (selectedConnectionId != -1)
    {
        getCurrentGraph().removeConnection(selectedConnectionId);
        selectedConnectionId = -1;
    }

    for (int id : selectedNodeIds)
    {
        getCurrentGraph().removeNode(id);
    }
    selectedNodeIds.clear();
    repaint();
}

void RelativisticCanvasComponent::spawnMessageBoxForNode(int targetNodeId, const std::string& msgText)
{
    auto targetNode = getCurrentGraph().getNode(targetNodeId);
    if (!targetNode) return;

    static int nextMsgId = 500;
    auto msgNode = RelativisticNodeFactory::createNode(nextMsgId++, "msg " + msgText);
    if (!msgNode) return;

    // Position message box slightly above and to the left of the target node
    msgNode->xPos = std::max(20.0f, targetNode->xPos - 40.0f);
    msgNode->yPos = std::max(40.0f, targetNode->yPos - 60.0f);

    getCurrentGraph().addNode(msgNode);
    // Connect msg node outlet 0 to target node inlet 0
    getCurrentGraph().addConnection(msgNode->getId(), 0, targetNodeId, 0);

    repaint();
}

bool RelativisticCanvasComponent::keyPressed(const juce::KeyPress& key)
{
    bool isCmdOrCtrl = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();
    int code = std::tolower(key.getKeyCode());

    // Cmd + A: Select All Nodes
    if (isCmdOrCtrl && code == 'a' && !isEditingObject)
    {
        selectAllNodes();
        return true;
    }

    // Cmd + C: Copy Selected Nodes
    if (isCmdOrCtrl && code == 'c' && !isEditingObject)
    {
        copySelectedNodes();
        return true;
    }

    // Cmd + X: Cut Selected Nodes
    if (isCmdOrCtrl && code == 'x' && !isEditingObject)
    {
        cutSelectedNodes();
        return true;
    }

    // Cmd + V: Paste Clipboard Nodes
    if (isCmdOrCtrl && code == 'v' && !isEditingObject)
    {
        pasteClipboardNodes();
        return true;
    }

    // Cmd + D: Duplicate Selected Nodes
    if (isCmdOrCtrl && code == 'd' && !isEditingObject)
    {
        duplicateSelectedNodes();
        return true;
    }

    // Cmd + 0 or Home key: Recenter Canvas View
    if ((isCmdOrCtrl && key.getKeyCode() == '0') || key.getKeyCode() == juce::KeyPress::homeKey)
    {
        recenterView();
        return true;
    }

    // Cmd + 1: Spawn object box at cursor
    if (isCmdOrCtrl && key.getKeyCode() == '1')
    {
        spawnObjectEditorAt(lastMousePos);
        return true;
    }

    // Enter when blank space selected: Spawn object box at cursor
    if (key.getKeyCode() == juce::KeyPress::returnKey && selectedNodeIds.empty() && !isEditingObject)
    {
        spawnObjectEditorAt(lastMousePos);
        return true;
    }

    // Esc or Delete/Backspace key: Delete selected nodes or selected connection cable
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        if (isEditingObject)
        {
            cancelObjectCreation();
            return true;
        }
        else
        {
            deleteSelectedNodes();
            return true;
        }
    }
    else if ((key.getKeyCode() == juce::KeyPress::backspaceKey || key.getKeyCode() == juce::KeyPress::deleteKey) && !isEditingObject)
    {
        deleteSelectedNodes();
        return true;
    }

    // Arrow keys: Nudge and move all selected objects together!
    if (!isEditingObject && !selectedNodeIds.empty())
    {
        float step = key.getModifiers().isShiftDown() ? 50.0f : 10.0f;
        if (key.getKeyCode() == juce::KeyPress::leftKey)
        {
            for (int id : selectedNodeIds)
            {
                auto n = getCurrentGraph().getNode(id);
                if (n) n->xPos -= step;
            }
            repaint();
            return true;
        }
        else if (key.getKeyCode() == juce::KeyPress::rightKey)
        {
            for (int id : selectedNodeIds)
            {
                auto n = getCurrentGraph().getNode(id);
                if (n) n->xPos += step;
            }
            repaint();
            return true;
        }
        else if (key.getKeyCode() == juce::KeyPress::upKey)
        {
            for (int id : selectedNodeIds)
            {
                auto n = getCurrentGraph().getNode(id);
                if (n) n->yPos -= step;
            }
            repaint();
            return true;
        }
        else if (key.getKeyCode() == juce::KeyPress::downKey)
        {
            for (int id : selectedNodeIds)
            {
                auto n = getCurrentGraph().getNode(id);
                if (n) n->yPos += step;
            }
            repaint();
            return true;
        }
    }

    return false;
}

void RelativisticCanvasComponent::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    auto pos = e.position;
    lastMousePos = pos;
    auto& currGraph = getCurrentGraph();

    if (isEditingObject && !objectEditor.getBounds().contains(pos.toInt()))
    {
        commitObjectCreation();
    }

    // Check click on Breadcrumb back button
    if (pos.y < 28.0f && graphStack.size() > 1)
    {
        popSubGraphView();
        return;
    }

    // Middle-click, Right-click on blank canvas, or Space+Click pans canvas!
    if (e.mods.isMiddleButtonDown() || juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey))
    {
        isPanning = true;
        panStartMouse = pos;
        panStartOffset = { viewOffsetX, viewOffsetY };
        return;
    }

    // 1. Check if clicked a Port (Outlet OR Inlet for bi-directional dragging!)
    for (const auto& node : currGraph.getNodes())
    {
        // Check Outlets
        for (int o = 0; o < static_cast<int>(node->getOutlets().size()); ++o)
        {
            auto p = getPortPos(*node, true, o);
            if (p.getDistanceFrom(pos) < 12.0f)
            {
                clearSelection();
                draggingPortNodeId = node->getId();
                draggingPortIdx = o;
                isDraggingFromOutlet = true;
                dragCurrentPos = pos;
                repaint();
                return;
            }
        }

        // Check Inlets
        for (int i = 0; i < static_cast<int>(node->getInlets().size()); ++i)
        {
            auto p = getPortPos(*node, false, i);
            if (p.getDistanceFrom(pos) < 12.0f)
            {
                clearSelection();
                draggingPortNodeId = node->getId();
                draggingPortIdx = i;
                isDraggingFromOutlet = false;
                dragCurrentPos = pos;
                repaint();
                return;
            }
        }
    }

    // 2. Check if clicked a Node panel
    bool isShift = e.mods.isShiftDown();
    for (const auto& node : currGraph.getNodes())
    {
        auto b = getNodeBounds(*node);
        if (b.contains(pos))
        {
            if (isShift)
            {
                toggleNodeSelection(node->getId());
            }
            else if (!isNodeSelected(node->getId()))
            {
                clearSelection();
                selectNode(node->getId());
            }

            multiNodeDragStarts.clear();
            for (int id : selectedNodeIds)
            {
                auto n = currGraph.getNode(id);
                if (n) multiNodeDragStarts[id] = { n->xPos, n->yPos };
            }
            nodeDragStartPos = pos;

            if (onNodeSelected) onNodeSelected(node);

            std::string sym = node->getSymbol();
            std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

            // Check if clicked Realtime Toggle Button [👁] (Only for Audio / Time nodes)
            if (!isControlGuiSymbol(sym) && !isControlLogicSymbol(sym) && !isSequencerSymbol(sym))
            {
                auto headerRect = b.withHeight(22.0f);
                auto toggleBtnRect = headerRect.removeFromRight(22.0f).reduced(2.0f);
                if (toggleBtnRect.contains(pos))
                {
                    node->toggleRealtimeDisplay();
                    repaint();
                    return;
                }

                // Check if clicked Scope Mode Button (Only for Time Sculptors)
                if (isTimeSculptorSymbol(sym))
                {
                    auto modeBtnRect = headerRect.removeFromRight(46.0f).reduced(2.0f);
                    if (modeBtnRect.contains(pos))
                    {
                        node->cycleTimeVarMode();
                        repaint();
                        return;
                    }
                }
            }
            else if (isSequencerSymbol(sym))
            {
                auto headerRect = b.withHeight(22.0f);
                auto editBtnRect = headerRect.removeFromRight(22.0f).reduced(2.0f);
                if (editBtnRect.contains(pos))
                {
                    spawnObjectEditorForNode(node->getId());
                    return;
                }
            }

            // Check if clicked Resize Handle (bottom-right 14x14 px corner)
            auto gripRect = juce::Rectangle<float>(b.getRight() - 14.0f, b.getBottom() - 14.0f, 14.0f, 14.0f);
            if (gripRect.contains(pos))
            {
                isResizingNode = true;
                nodeResizeStartSize = { node->width, node->height };
                repaint();
                return;
            }

            // Double-click on any node opens inline text editor for editing node parameters/text
            if (e.getNumberOfClicks() >= 2)
            {
                spawnObjectEditorForNode(node->getId());
                return;
            }

            // Interactive Click Triggers for GUI Control Nodes!
            if (sym == "bang" || sym == "bng")
            {
                auto bNode = std::dynamic_pointer_cast<BangNode>(node);
                if (bNode) bNode->triggerBang();
                repaint();
                return;
            }
            else if (sym == "toggle" || sym == "tgl")
            {
                auto tNode = std::dynamic_pointer_cast<ToggleNode>(node);
                if (tNode) tNode->toggleState();
                repaint();
                return;
            }
            else if (sym == "radio" || sym == "hradio" || sym == "vradio")
            {
                auto rNode = std::dynamic_pointer_cast<RadioNode>(node);
                if (rNode)
                {
                    float relX = pos.x - b.getX();
                    int opts = rNode->getNumOptions();
                    int idx = std::clamp(static_cast<int>(relX / (b.getWidth() / static_cast<float>(opts))), 0, opts - 1);
                    rNode->selectOption(idx);
                }
                repaint();
                return;
            }
            else if (sym == "msg" || sym == "message")
            {
                auto mNode = std::dynamic_pointer_cast<MessageNode>(node);
                if (mNode) mNode->triggerMessage();
                repaint();
                return;
            }

            // ONLY DISPATCH MESSAGE ON Cmd-Click (or Ctrl-Click)! Normal click is strictly for editing/selection!
            if (e.mods.isCommandDown() || e.mods.isCtrlDown())
            {
                juce::String symStr = juce::String(node->getSymbol()).toLowerCase();
                if (symStr == "msg" || symStr == "message" || symStr == "bng" || symStr == "bang" || symStr == "number" || symStr == "radio" || symStr == "toggle")
                {
                    auto msgNode = std::dynamic_pointer_cast<MessageNode>(node);
                    std::string msgText = msgNode ? msgNode->getMessageText() : node->getLabel();
                    if (msgText.empty()) msgText = "play";

                    for (const auto& conn : currGraph.getConnections())
                    {
                        if (conn.sourceNodeId == node->getId())
                        {
                            auto dest = currGraph.getNode(conn.destNodeId);
                            if (dest)
                            {
                                dest->receiveMessage(msgText);
                            }
                        }
                    }
                }
            }
            repaint();
            return;
        }
    }

    // 3. Check if clicked near a Patch Connection Cable (Cable Selection)
    for (const auto& conn : currGraph.getConnections())
    {
        auto srcNode = currGraph.getNode(conn.sourceNodeId);
        auto destNode = currGraph.getNode(conn.destNodeId);
        if (!srcNode || !destNode) continue;

        auto p1 = getPortPos(*srcNode, true, conn.sourcePortIndex);
        auto p2 = getPortPos(*destNode, false, conn.destPortIndex);

        // Distance from point to line segment p1-p2
        juce::Line<float> line(p1, p2);
        if (line.findNearestPointTo(pos).getDistanceFrom(pos) < 10.0f)
        {
            clearSelection();
            selectedConnectionId = conn.connectionId;

            // Right-click options on connection cable
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu m;
                m.addItem(1, "Delete Cable Connection");
                m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, connId = conn.connectionId](int res) {
                    if (res == 1) {
                        getCurrentGraph().removeConnection(connId);
                        selectedConnectionId = -1;
                        repaint();
                    }
                });
            }

            repaint();
            return;
        }
    }

    // 4. Clicked blank space: Start marquee lasso selection box
    if (!isShift)
    {
        clearSelection();
        if (onNodeSelected) onNodeSelected(nullptr);
    }
    isMarqueeSelecting = true;
    marqueeStartPos = pos;
    marqueeRect = { pos.x, pos.y, 0.0f, 0.0f };
    repaint();
}

void RelativisticCanvasComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto pos = e.position;
    auto& currGraph = getCurrentGraph();
    bool clickedNode = false;

    for (const auto& node : currGraph.getNodes())
    {
        auto b = getNodeBounds(*node);
        if (b.contains(pos))
        {
            clickedNode = true;
            // If composite node, drill down into its internal sub-graph
            if (node->getSymbol() == "patch~" || node->getSymbol() == "osc.patch~")
            {
                auto compNode = std::dynamic_pointer_cast<CompositeNode>(node);
                if (compNode)
                {
                    pushSubGraphView(&compNode->getInternalSubGraph(), compNode->getLabel());
                }
            }
            else
            {
                // Double-click existing node to re-edit its parameters!
                spawnObjectEditorForNode(node->getId());
            }
            break;
        }
    }

    // Double-clicking blank canvas spawns a new object creation box!
    if (!clickedNode)
    {
        spawnObjectEditorAt(pos);
    }
}

void RelativisticCanvasComponent::mouseDrag(const juce::MouseEvent& e)
{
    auto pos = e.position;
    auto& currGraph = getCurrentGraph();

    if (isPanning)
    {
        viewOffsetX = panStartOffset.x + (pos.x - panStartMouse.x);
        viewOffsetY = panStartOffset.y + (pos.y - panStartMouse.y);
        repaint();
        return;
    }

    if (isMarqueeSelecting)
    {
        marqueeRect = juce::Rectangle<float>::leftTopRightBottom(std::min(marqueeStartPos.x, pos.x), std::min(marqueeStartPos.y, pos.y), std::max(marqueeStartPos.x, pos.x), std::max(marqueeStartPos.y, pos.y));
        if (!e.mods.isShiftDown()) selectedNodeIds.clear();
        for (const auto& node : currGraph.getNodes())
        {
            if (getNodeBounds(*node).intersects(marqueeRect))
            {
                selectNode(node->getId());
            }
        }
        repaint();
        return;
    }

    if (isResizingNode && !selectedNodeIds.empty())
    {
        int targetId = *selectedNodeIds.begin();
        auto node = currGraph.getNode(targetId);
        if (node)
        {
            float newW = std::max(120.0f, nodeResizeStartSize.x + static_cast<float>(e.getDistanceFromDragStartX()));
            float newH = std::max(45.0f, nodeResizeStartSize.y + static_cast<float>(e.getDistanceFromDragStartY()));
            node->width = newW;
            node->height = newH;
            repaint();
            return;
        }
    }

    if (draggingPortNodeId != -1)
    {
        dragCurrentPos = pos;
        repaint();
    }
    else if (!multiNodeDragStarts.empty())
    {
        float dx = static_cast<float>(e.getDistanceFromDragStartX());
        float dy = static_cast<float>(e.getDistanceFromDragStartY());
        for (const auto& kv : multiNodeDragStarts)
        {
            auto node = currGraph.getNode(kv.first);
            if (node)
            {
                node->xPos = kv.second.x + dx;
                node->yPos = kv.second.y + dy;
            }
        }
        repaint();
    }
}

void RelativisticCanvasComponent::mouseUp(const juce::MouseEvent& e)
{
    auto pos = e.position;
    auto& currGraph = getCurrentGraph();

    if (isPanning)
    {
        isPanning = false;
        repaint();
        return;
    }

    if (isResizingNode)
    {
        isResizingNode = false;
        repaint();
        return;
    }

    if (isMarqueeSelecting)
    {
        isMarqueeSelecting = false;
        marqueeRect = {};
        repaint();
    }
    multiNodeDragStarts.clear();

    if (draggingPortNodeId != -1)
    {
        auto isCompatible = [](PortDataType src, PortDataType dest) -> bool {
            if (src == dest) return true;
            if ((src == PortDataType::Time && dest == PortDataType::Audio) ||
                (src == PortDataType::Audio && dest == PortDataType::Time)) return true;
            if (src == PortDataType::Message && dest == PortDataType::Time) return true;
            return false;
        };

        for (const auto& node : currGraph.getNodes())
        {
            if (node->getId() == draggingPortNodeId) continue;

            if (isDraggingFromOutlet)
            {
                // Dragged from Outlet -> Drop on Inlet
                for (int i = 0; i < static_cast<int>(node->getInlets().size()); ++i)
                {
                    auto p = getPortPos(*node, false, i);
                    if (p.getDistanceFrom(pos) < 15.0f)
                    {
                        auto srcNode = currGraph.getNode(draggingPortNodeId);
                        if (srcNode && draggingPortIdx < static_cast<int>(srcNode->getOutlets().size()))
                        {
                            auto srcType = srcNode->getOutlets()[static_cast<size_t>(draggingPortIdx)].dataType;
                            auto destType = node->getInlets()[static_cast<size_t>(i)].dataType;
                            if (isCompatible(srcType, destType))
                            {
                                currGraph.addConnection(draggingPortNodeId, draggingPortIdx, node->getId(), i);
                            }
                        }
                        break;
                    }
                }
            }
            else
            {
                // Dragged from Inlet -> Drop on Outlet
                for (int o = 0; o < static_cast<int>(node->getOutlets().size()); ++o)
                {
                    auto p = getPortPos(*node, true, o);
                    if (p.getDistanceFrom(pos) < 15.0f)
                    {
                        auto destNode = currGraph.getNode(draggingPortNodeId);
                        if (destNode && draggingPortIdx < static_cast<int>(destNode->getInlets().size()))
                        {
                            auto srcType = node->getOutlets()[static_cast<size_t>(o)].dataType;
                            auto destType = destNode->getInlets()[static_cast<size_t>(draggingPortIdx)].dataType;
                            if (isCompatible(srcType, destType))
                            {
                                currGraph.addConnection(node->getId(), o, draggingPortNodeId, draggingPortIdx);
                            }
                        }
                        break;
                    }
                }
            }
        }
        draggingPortNodeId = -1;
        draggingPortIdx = -1;
        repaint();
    }
}

void RelativisticCanvasComponent::recenterView()
{
    viewOffsetX = 0.0f;
    viewOffsetY = 0.0f;
    repaint();
}

void RelativisticCanvasComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(e);
    viewOffsetX += wheel.deltaX * 120.0f;
    viewOffsetY += wheel.deltaY * 120.0f;
    repaint();
}

} // namespace TimeDilationDAW
