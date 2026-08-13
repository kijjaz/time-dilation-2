# Relativistic Time & Composite Sub-Graph Specification

> **Project**: Time Dilation DAW 2  
> **Location**: `/Users/kijjaz/Desktop/Antigravity/2026/20260812 Time Dilation DAW 2/docs/CONCEPT_AND_COMPOSITE_NODES.md`  

---

## 1. What the Time Concept Does to Audio & Control Objects

Every node runs on a local coordinate time clock ($\tau$). Time context ($\gamma = d\tau / dt$) flows down purple cables and transforms downstream objects as follows:

```
                  ┌──────────────────────────────────────────────┐
                  │          Relativistic Time Cable             │
                  │   Gamma (Speed), Offset, Polyphonic Streams  │
                  └──────────────────────┬───────────────────────┘
                                         │
       ┌─────────────────────────────────┼─────────────────────────────────┐
       ▼                                 ▼                                 ▼
┌──────────────┐                 ┌──────────────┐                 ┌──────────────┐
│ Audio Objects│                 │Control Objects│                │ Time Objects │
└──────┬───────┘                 └──────┬───────┘                 └──────┬───────┘
       │                                 │                                 │
       ├─► Pitch Doppler Shift           ├─► Sequencer Step Rate           ├─► Lorentz Speed Addition
       ├─► Sub-sample Hermite Resample   ├─► Envelope Attack/Release Time  ├─► Multi-Stream Cloud Fanning
       ├─► Reverse Wave Buffer           ├─► LFO Modulation Frequency      ├─► Step Grid Quantization
       └─► Time-Warped Delay Length      └─► Trigger Impulse Rate          └─► Gravitational Freeze
```

### A. Effects on Audio Objects (`osc~`, `sampler~`, `delay~`, `tabread~`)
1. **Doppler Pitch Bending**: Resampling phase steps by $|\gamma|$ automatically shifts fundamental pitch and harmonics without phase discontinuities.
2. **Sub-Sample Fractional Hermite Resampling**: Non-integer time dilation rates interpolate continuous 4-point C1 cubic curves, keeping signals 100% anti-aliased.
3. **Temporal Reversal ($\gamma < 0$)**: Negative time speed steps phase backwards through array tables, playing audio backwards in real-time.
4. **Time-Warped Delay Buffer**: Delays stretch or compress feedback delay lengths smoothly in response to relativistic time contraction.

### B. Effects on Control Objects (`seq`, `seq.tidal`, `env~`, `lfo`)
1. **Relativistic Transport Speed**: Sequencers step through patterns at $\text{BPM} \cdot \gamma$. A $2.0\times$ time warp doubles sequence tempo.
2. **Time-Scaled Envelopes**: Envelope Attack, Decay, and Release times scale inversely with local time speed, keeping envelopes synchronized to warped rhythm grids.
3. **Gravitational Stasis ($\gamma = 0$)**: Freezes sequence position and envelope decay indefinitely until time resumes.

---

## 2. Composite Nodes: Constructing Big Objects from Smaller Sub-Objects (`[patch~]`)

A **Composite Node** (`[patch~]`) is a custom node whose internal logic is built by patching smaller core nodes together inside an isolated sub-graph (`RelativisticNodeGraph`).

```
┌────────────────────────────────────────────────────────────────────────┐
│ COMPOSITE NODE: [synth.voice~] (Represented as a single visual block)  │
│                                                                        │
│ Inlets: [timeIn], [note], [gate]      Outlets: [audioOut~]             │
│                                                                        │
│  INTERNAL SUB-GRAPH:                                                   │
│  [inlet note] ───► [mtof] ────► [osc~ sin] ────┐                       │
│                                     ▲          │                       │
│  [inlet timeIn] ────────────────────┘          ▼                       │
│  [inlet gate] ───► [env~ adsr] ────────────► [svf~] ───► [outlet out~] │
└────────────────────────────────────────────────────────────────────────┘
```

### C++ Sub-Graph Abstraction Architecture:

```cpp
class CompositeNode : public RelativisticNode
{
public:
    CompositeNode (int id, const std::string& patchName)
        : RelativisticNode (id, "patch~", patchName)
    {
        subGraph = std::make_unique<RelativisticNodeGraph>();
    }

    void loadSubPatch (const juce::File& patchFile)
    {
        // Deserializes internal sub-graph nodes and connections
    }

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        RelativisticNode::prepare (sampleRate, samplesPerBlock);
        subGraph->prepare (sampleRate, samplesPerBlock);
    }

    void process (int numSamples) override
    {
        // 1. Map Composite Node Inlets to Sub-Graph [inlet] nodes
        mapInletsToSubGraph();

        // 2. Process internal sub-graph block
        juce::AudioBuffer<float> subMaster (2, numSamples);
        subGraph->process (subMaster, numSamples);

        // 3. Map Sub-Graph [outlet~] nodes to Composite Node Outlets
        mapSubGraphToOutlets();
    }

private:
    std::unique_ptr<RelativisticNodeGraph> subGraph;
};
```

---

## 3. Reusability & Shared Preset Library (`.pdil` format)

Composite nodes can be saved as standard JSON files (`.pdil` = **Time Dilation Patch File**) and re-instantiated anywhere by name:

```json
{
  "compositeName": "synth.voice~",
  "inlets": ["timeIn", "note", "gate"],
  "outlets": ["out~"],
  "subGraph": {
    "nodes": [
      { "id": 1, "type": "mtof" },
      { "id": 2, "type": "osc~" },
      { "id": 3, "type": "env~" },
      { "id": 4, "type": "svf~" }
    ],
    "connections": [
      { "fromNode": 1, "fromPort": 0, "toNode": 2, "toPort": 1 },
      { "fromNode": 2, "fromPort": 0, "toNode": 4, "toPort": 0 },
      { "fromNode": 3, "fromPort": 0, "toNode": 4, "toPort": 1 }
    ]
  }
}
```

This completes the architectural loop: **Any complex synth, drum voice, or relativistic multi-effect can be built from atomic core nodes (`osc~`, `table`, `svf~`, `env~`), saved to disk, and reused as a single block!**
