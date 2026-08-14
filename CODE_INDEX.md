# Time Dilation DAW — Code Index & Symbol Navigation Map

This document maps all source files in `Source/` with their line numbers, classes, structs, methods, and architectural responsibilities for fast LLM comprehension and human navigation.

---

## 1. DSP Engine Layer (`Source/dsp/`)

### [TimePolyFrame.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/TimePolyFrame.h) (94 lines)
*Core multi-stream relativistic time representation.*
- **L11 - L43**: `struct TimePolyStream` — Individual relativistic timeline stream (`gamma`, `tau`, `couplingC`, `weight`, `windowEnv`).
  - `L22`: `getWindowEnvelope(double t)` — Raised cosine Hann/Tukey time window.
  - `L31`: `advance(double dt)` — Advances proper time $\tau$ according to speed factor $\gamma$.
- **L45 - L92**: `struct TimePolyFrame` — Composite multi-stream telemetry frame (`masterGamma`, `couplingC`, `masterPhase`, `activeStreamCount`).
  - `L63`: `reset()` — Clears and sets single stream with $\gamma = 1.0, \tau = 0.0$.
  - `L74`: `advanceFrame(double dt)` — Propagates multi-stream proper time frame across audio blocks.

---

### [RelativisticNodeGraph.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/RelativisticNodeGraph.h) (561 lines)
*Core node graph, node base class, connection structures, and latency compensation.*
- **L18 - L48**: `enum class PortDataType`, `struct Inlet`, `struct Outlet`, `struct Connection`.
- **L50 - L220**: `class RelativisticNode` — Universal base class for all modular processing and GUI nodes.
  - `L65`: Scope Display Enums (`ScopeDisplayType`, `ScopeRenderMode`, `TimeScopeVariable`).
  - `L110`: `prepare(sampleRate, samplesPerBlock)`
  - `L111`: `process(numSamples)`
  - `L112`: `receiveMessage(const std::string& message)`
  - `L135`: `getVolumeDb()`, `setVolumeDb()`, `getOutputVolume()`, `setOutputVolume()`
  - `L180`: `pushScopeSample(float sample)`, `pushTimeScopeSample(float gamma, float tau)`
- **L225 - L340**: `class DynamicLatencyCompensationEngine` — Pre-causal dynamic look-ahead and Hermite fractional delay interpolation.
- **L345 - L430**: `class PreCausalCircularBuffer` — Lock-free circular audio buffer with cubic Hermite reading.
- **L435 - L560**: `class RelativisticNodeGraph` — Graph container, topological sorting, audio block processing, sub-graph routing.

---

### [RelativisticNodeGraph.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/RelativisticNodeGraph.cpp) (867 lines)
*Implementation of graph topology, block processing loop, and connection lifecycle.*
- **L12 - L80**: Graph node insertion, removal, selection, and connection verification.
- **L90 - L180**: Serialization (`serializeGraph`, `deserializeGraph`).
- **L190 - L280**: Dynamic Latency Compensation calculations (`updateDemand`, `advanceSampleSmooth`).
- **L290 - L410**: Pre-Causal Buffer Hermite cubic interpolation methods (`readFutureSampleHermiteAtPos`).
- **L420 - L640**: Node Graph block processing & topological execution loop (`processGraph`).
- **L673 - L710**: `out~` master channel summation (Inlet 1 $\rightarrow$ L, Inlet 2 $\rightarrow$ R, clean linear gain).
- **L720 - L760**: Real-time scope buffer capture loop.

---

### [RelativisticSoundNodes.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/RelativisticSoundNodes.h) (395 lines)
*Class declarations for all audio DSP nodes and Pure Data style GUI control nodes.*
- **L14 - L26**: `class TableManager` — Shared audio sample buffer repository.
- **L28 - L48**: `class OscNode` (`osc~`) — PolyBLEP anti-aliased multi-waveform oscillator.
- **L50 - L70**: `class LadderNode` (`ladder~`) — 4-pole Moog VA ladder filter with Doppler cutoff modulation.
- **L72 - L92**: `class DriveNode` (`drive~` / `saturate~`) — Hyperbolic tangent tubesaturator.
- **L94 - L114**: `class PluckNode` (`pluck~`) — Karplus-Strong physical string model.
- **L116 - L136**: `class DelayNode` (`delay~`) — Feedback delay line with Hermite interpolation.
- **L138 - L158**: `class SVFNode` (`svf~`) — State Variable Filter (LP/HP/BP/Notch).
- **L160 - L178**: `class MessageNode` (`msg`) — Pure Data flag message box.
- **L180 - L196**: `class BangNode` (`bang` / `bng`) — Square target circle box.
- **L198 - L215**: `class ToggleNode` (`toggle` / `tgl`) — `[X]` / `[ ]` boolean switch.
- **L217 - L235**: `class NumberNode` (`number` / `num`) — Slanted notch numeric box.
- **L237 - L254**: `class SymbolNode` (`symbol` / `sym`) — String text message box.
- **L256 - L275**: `class RadioNode` (`radio` / `hradio` / `vradio`) — Radio buttons strip.
- **L277 - L295**: `class DisplayNode` (`display` / `disp` / `print`) — Live terminal display readout.
- **L297 - L320**: `class OutNode` (`out~`) — Master stereo output, RMS metering, waveform monitoring.
- **L322 - L342**: `class GravRedshiftOscNode` (`time.grav.osc~`) — General Relativity redshift oscillator.
- **L344 - L368**: `class LorentzFilterNode` (`time.lorentz~`) — Formant Lorentz velocity filter ($v/c$).
- **L370 - L395**: `class MeterNode` (`meter~` / `vu~`) & `class SpectrogramNode` (`spectrogram~` / `spec~`).

---

### [RelativisticSoundNodes.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/RelativisticSoundNodes.cpp) (1397 lines)
*DSP processing algorithms and message handlers for all sound and control nodes.*
- **L39 - L145**: `OscNode` (`osc~`) — PolyBLEP anti-aliased synthesis (Sine, Saw, Square, Triangle).
- **L147 - L230**: `TableNode` (`table`) & `TabReadNode` (`tabread~`) — Sample buffer interpolation.
- **L232 - L305**: `LadderNode` (`ladder~`) — 4-stage Moog ladder differential equation modeling.
- **L307 - L340**: `DelayNode` (`delay~`) — Cubic Hermite fractional delay line.
- **L342 - L420**: `SVFNode` (`svf~`) — Chamberlin state-variable filter topology.
- **L422 - L480**: `DriveNode` (`drive~`) — WaveShaping tube saturation.
- **L482 - L540**: `PluckNode` (`pluck~`) — Karplus-Strong string algorithm.
- **L542 - L575**: `MessageNode` (`msg`) — Message trigger and emission.
- **L577 - L615**: `BangNode` (`bang` / `bng`) — Bang pulse and flash timer animation.
- **L617 - L660**: `ToggleNode` (`toggle` / `tgl`) — Boolean state switching.
- **L662 - L705**: `NumberNode` (`number` / `num`) — Numeric parsing and setter/getter logic.
- **L707 - L745**: `SymbolNode` (`symbol` / `sym`) — String storage and dispatching.
- **L747 - L785**: `RadioNode` (`radio`) — Multi-option radio selection.
- **L787 - L815**: `DisplayNode` (`display`) — Dynamic string readout.
- **L817 - L940**: `receiveMessage` overrides for all nodes (Standard property protocol).
- **L942 - L1005**: `OutNode` (`out~`) — Clean linear pass-through, RMS and waveform capture, default `-6.0 dB`.
- **L1007 - L1080**: `GravRedshiftOscNode` (`time.grav.osc~`) — Einstein gravitational frequency redshift.
- **L1082 - L1180**: `LorentzFilterNode` (`time.lorentz~`) — Relativistic Doppler velocity filtering.
- **L1182 - L1397**: `MeterNode` & `SpectrogramNode` — Peak/RMS/LUFS metering and 256-bin FFT waterfall.

---

### [RelativisticSequencers.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/RelativisticSequencers.h) & [RelativisticSequencers.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/RelativisticSequencers.cpp)
*Timeline transformation and sequencing nodes.*
- `TimeWarpNode` (`time.warp~`) — Velocity acceleration/deceleration ($\gamma$).
- `TimeLFONode` (`time.lfo~`) — Relativistic LFO modulating $\gamma(t)$.
- `TimeRetroNode` (`time.retro~`) — Time reversal / negative velocity ($\gamma < 0$).
- `TimeStasisNode` (`time.stasis~`) — Zero proper time freeze ($\gamma = 0$).
- `TimeMathNode` (`time.math~`) — Polynomial and scaling arithmetic on time frames.
- `TimeScopeNode` (`time.scope~`) — Telemetry plotter.
- `SeqNode` (`seq`) — Step sequencer advancing via relativistic proper time clock.
- `MtofNode` (`mtof` / `mtof~`) — MIDI Note Number $\rightarrow$ Frequency ($f = 440 \cdot 2^{(m-69)/12}$).
- `FtomNode` (`ftom` / `ftom~`) — Frequency in Hz $\rightarrow$ MIDI Note ($m = 69 + 12 \log_2(f/440)$).
- `TransportNode` (`time.transport~`) — Relativistic master DAW timeline transport clock.

---

### [RelativisticNodeFactory.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/dsp/RelativisticNodeFactory.cpp) (232 lines)
*Factory registry instantiating nodes from symbol strings (e.g. `osc~ sin 440`, `msg play`, `toggle 1`, `mtof~ 60`).*

---

## 2. User Interface Layer (`Source/gui/`)

### [RelativisticCanvasComponent.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/RelativisticCanvasComponent.h) & [RelativisticCanvasComponent.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/RelativisticCanvasComponent.cpp) (1703 lines)
*Interactive modular canvas, cable patching, custom node drawing, autocomplete popup.*
- **L30 - L80**: `allCatalogueObjects` — Autocomplete catalog of 30+ objects with descriptions.
- **L135 - L250**: Canvas background grid, breadcrumbs, selection marquee.
- **L255 - L405**: Custom GUI object shapes (Flag cut for `msg`, circle for `bang`, `[X]` for `toggle`, slanted notch for `number`, radio strip, display screen).
- **L410 - L815**: Audio/Time DSP node card rendering (Header, eye toggle `[👁]`, mode button, 2D/3D scope display, VU meters, spectrogram).
- **L825 - L855**: Inlets (Message/Time/Audio) and Outlets rendering.
- **L867 - L940**: Real-time object autocomplete popup list (`updateAutocompleteSuggestions`).
- **L945 - L1105**: Keyboard Shortcuts (`Cmd+1` create object, `Cmd+C`/`Cmd+V` copy/paste, `Cmd+D` duplicate, `Cmd+0` recenter).
- **L1107 - L1450**: `mouseDown`, `mouseDrag`, `mouseUp`, `mouseDoubleClick` — Cable dragging, node dragging, click triggers for `msg`/`bang`/`toggle`/`radio`, resize grip.

---

### [NodeInspectorComponent.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/NodeInspectorComponent.h) & [NodeInspectorComponent.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/NodeInspectorComponent.cpp) (959 lines)
*Context-sensitive property inspector, telemetry monitor, inlet/outlet function docs, and one-click trigger buttons.*
- **L121 - L185**: Node Position, Width, Height, and Real-time Telemetry header.
- **L186 - L275**: Category-aware controls (Hides volume & scopes for GUI control objects; shows `Master Volume (dB)` for `out~`; shows `Gain Multiplier (x)` for audio nodes).
- **L285 - L540**: Dedicated node parameters (e.g. `Cutoff`/`Res` for `ladder~`, `Delay`/`Feedback` for `delay~`, `v/c` for `lorentz~`, `Note` for `mtof`, etc.).
- **L542 - L650**: `getPortFunctionDesc` — Explicit port function explanations and domain color tags.
- **L680 - L700**: Dynamic one-click method trigger buttons (`+ msg <cmd>`).

---

### [WorkstationContainerComponent.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/WorkstationContainerComponent.h) & [WorkstationContainerComponent.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/WorkstationContainerComponent.cpp) (724 lines)
*Master DAW window container hosting Top Navigation Bar, Master Transport Bar, Modular Canvas, Timeline, Split View, and Inspector.*
- **L30 - L180**: Transport controls (BPM, Signature, DSP ON/OFF, View switches).
- **L235 - L280**: Default patch construction (`setupDefaultPatch`).
- **L400 - L450**: `prepareToPlay`, `processBlock` audio device callbacks.

---

### [CarbonGoldLookAndFeel.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/CarbonGoldLookAndFeel.h) & [CarbonGoldLookAndFeel.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/gui/CarbonGoldLookAndFeel.cpp) (70 lines)
*Theme palette and vector styling:*
- `carbonDark`: `#0c0e12` (Background Canvas)
- `slatePanel`: `#161b24` (Node Cards)
- `goldAccent`: `#d4af37` / `#ffd700` (Control & Message Domain)
- `cyberCyan`: `#00e5ff` (Audio Domain `~`)
- `royalViolet`: `#8a2be2` (Relativistic Time Domain)

---

## 3. Utilities & CLI Automation (`Source/utils/`)

### [TerminalCommandProcessor.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/utils/TerminalCommandProcessor.h) & [TerminalCommandProcessor.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/utils/TerminalCommandProcessor.cpp) (369 lines)
*CLI scripting engine executing commands like `--cmd="add osc~ sin 440"`, `--cmd="connect 1 0 2 0"`, `--cmd="nodes"`.*

---

### [AgentTestRunner.h](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/utils/AgentTestRunner.h) & [AgentTestRunner.cpp](file:///Users/kijjaz/Desktop/Antigravity/2026/20260812%20Time%20Dilation%20DAW%202/Source/utils/AgentTestRunner.cpp) (882 lines)
*Automated headless integration test suite validating node creation, cable routing, time dilation audio processing, and serialization.*
