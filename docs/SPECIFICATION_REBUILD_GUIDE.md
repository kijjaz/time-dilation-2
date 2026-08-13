# Time Dilation DAW 2 — Complete Technical Specification & Clean Rebuild Guide

> **Target Directory**: `/Users/kijjaz/Desktop/Antigravity/2026/20260812 Time Dilation DAW 2`  
> **Architecture Version**: 0.0.3 Clean Standalone  
> **Platform**: macOS (JUCE 7 / C++20 Standalone Workstation & VST3/AU Plugin)  

---

## 1. Executive Concept

**Time Dilation DAW** is a state-of-the-art Relativistic Modular Audio Workstation built with C++20 and JUCE 7. It unifies top-down visual block diagram patching with bottom-up C++ DSP expression coding.

Every node in the workspace operates on its own **local coordinate time clock line** ($\tau$), allowing dynamic relativistic continuous time warping (`time.warp~`), temporal reversal (`time.retro~`), gravitational stasis (`time.stasis~`), relativistic grid quantization (`time.quantize~`), and Lorentz velocity addition composition (`time.math~`).

---

## 2. Directory Layout of the Clean Rebuild Project

To build the new standalone project completely isolated from older legacy files, assemble the project with the following file tree:

```
20260812 Time Dilation DAW 2/
├── CMakeLists.txt                         # Standalone JUCE 7 build file
├── README.md                              # Workstation overview
├── docs/
│   ├── SPECIFICATION_REBUILD_GUIDE.md     # This comprehensive rebuild manual
│   └── MATH_THEORY_GUIDE.md               # Complete physics & DSP math equations
├── math/                                  # 28 formal LaTeX mathematical papers
│   ├── 01_relativistic_time_dilation.tex
│   ├── 05_feedback_loop_stability.tex
│   ├── 13_lorentz_time_signal_composition.tex
│   └── future_lookahead_causality.tex
└── Source/
    ├── Main.cpp                           # App entry point & JUCE application class
    ├── dsp/
    │   ├── TimePolyFrame.h               # 1,024-stream polyphonic time frame container
    │   ├── RelativisticNodeGraph.h       # Core Node, Port, PatchConnection & Graph Engine
    │   ├── RelativisticNodeGraph.cpp
    │   ├── RelativisticNodeFactory.h     # Factory for instantiating all 35+ node symbols
    │   ├── RelativisticNodeFactory.cpp
    │   ├── RelativisticSoundNodes.h      # osc~, table, tabread~, svf~, delay~, out~ nodes
    │   ├── RelativisticSoundNodes.cpp
    │   ├── RelativisticSequencers.h      # seq, seq.tidal, mtof, ftom nodes
    │   └── RelativisticSequencers.cpp
    ├── gui/
    │   ├── CarbonGoldLookAndFeel.h       # Slate & Gold UI LookAndFeel styling tokens
    │   ├── CarbonGoldLookAndFeel.cpp
    │   ├── RelativisticCanvasComponent.h # Node canvas, catenary cables, selection halos
    │   ├── RelativisticCanvasComponent.cpp
    │   ├── WorkstationContainerComponent.h # Top-level workstation window layout
    │   └── WorkstationContainerComponent.cpp
    └── utils/
        ├── AgentTestRunner.h             # Headless WAV rendering & PNG snapshot renderer
        └── AgentTestRunner.cpp
```

---

## 3. Node Specifications & Command Reference (35+ Symbols)

### Relativistic Time Suite (`time.*`)
- `time.warp~`: Multiplies incoming coordinate time speed ($\gamma_{\text{out}} = \gamma_{\text{in}} \cdot \text{factor}$).
- `time.retro~`: Reverses direction of local coordinate time ($\gamma_{\text{out}} = -\gamma_{\text{in}}$).
- `time.stasis~`: Freezes local coordinate time ($\gamma = 0.0$). Holds current state.
- `time.quantize~`: Quantizes continuous time into step grid intervals ($1/16\text{th}, 1/8\text{th}, 1/4\text{th}$).
- `time.math~`: Lorentz composition of two time streams ($\gamma = \frac{\gamma_1 + \gamma_2}{1 + \gamma_1 \gamma_2}$).
- `time.grain~`: Fans single time stream into cloud of $N$ (up to 1,024) micro-time streams.

### Pure Data Style Shared Arrays (`table`, `tabread~`, `tabwrite~`)
- `table`: Named shared float array for storing waveforms, custom audio samples, or step sequences.
- `tabread`: Reads float value from named `table` at control-rate index.
- `tabwrite`: Writes float value to named `table` at control-rate index.
- `tabread~`: Audio-rate interpolated lookup reader from named `table`.
- `tabwrite~`: Audio-rate live recorder into named `table`.

### Sound Generators (`osc~`, `sampler~`, `drum.machine~`)
- `osc~`: Multi-waveform anti-aliased oscillator with Sub-Sample Cubic Hermite Interpolation. Responds dynamically to local time speed $\gamma$ (advances phase at 1.0x default when timeIn is unconnected).
- `sampler~`: Relativistic audio sample player reading from named `table` array across multiple time streams.
- `drum.machine~`: 8-voice analog drum synthesis engine (Kick, Snare, Hi-Hat, Clap, Tom, Rimshot, Cowbell, Cymbal).

### Logic & Processing (`seq`, `seq.tidal`, `mtof`, `svf~`, `delay~`, `out~`)
- `seq`: Pattern step sequencer with customizable note velocity and gate outputs.
- `seq.tidal`: TidalCycles mini-notation pattern parser (`"scale 'minor' 0 [3 5] 7 10"`, `~` rests).
- `mtof` / `ftom`: MIDI Note $\leftrightarrow$ Frequency converters.
- `svf~`: Chamberlin 2-Pole State-Variable Filter (Simultaneous Lowpass, Highpass, Bandpass, Notch).
- `delay~`: Relativistic delay line buffer with time-warped delay tap length.
- `out~`: Master stereo audio output node.

---

## 4. Building & Testing the New Clean Project

Run the following commands in terminal:

```bash
cd "/Users/kijjaz/Desktop/Antigravity/2026/20260812 Time Dilation DAW 2"

# 1. Generate CMake Build Directory
mkdir build && cd build
cmake ..

# 2. Compile Standalone Application
cmake --build . -j8

# 3. Execute Autonomous Test Suite & PNG Image Generator
./TimeDilationDAW --agent-test \
                  --capture-screen=artifacts/clean_render.png \
                  --telemetry=artifacts/clean_telemetry.json \
                  --auto-exit
```
