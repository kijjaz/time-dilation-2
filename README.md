# Time Dilation DAW 2 (Clean Relativistic Workstation Rebuild)

> **Producer & Lead Architect**: Kijjaz  
> **Location**: `/Users/kijjaz/Desktop/Antigravity/2026/20260812 Time Dilation DAW 2`  
> **Aesthetic System**: Carbon & Gold + Slate Sci-Fi Workstation  

---

## Overview

Welcome to **Time Dilation DAW 2**, a clean standalone C++20 / JUCE 7 implementation of the Relativistic Modular Audio Workstation.

In this workspace, legacy code clutter has been stripped away, leaving an elegant architecture where:
- Every node processes on its own **local coordinate time clock line** ($\tau = \gamma \cdot t$).
- Time streams flow across **Royal Violet (`#8b5cf6`)** time cables supporting up to 1,024 polyphonic stream clouds (`TimePolyFrame`).
- Audio signals flow across **Cyber Cyan (`#06b6d4`)** audio cables with 32-bit linear headroom.
- Patch connections feature Tarjan SCC cycle detection with 1-block delay buffers (`previousBlockBuffer`) for **100% feedback loop stability**.

---

## Documentation Index

- [`docs/SPECIFICATION_REBUILD_GUIDE.md`](docs/SPECIFICATION_REBUILD_GUIDE.md): Complete architecture specification, file tree, 35+ node symbol reference table, and rebuild instructions.
- [`docs/MATH_THEORY_GUIDE.md`](docs/MATH_THEORY_GUIDE.md): Mathematical calculations for Relativistic Time Dilation ($\gamma$), Lorentz Velocity Addition, 4-Point Cubic Hermite Interpolation, and SVF filter math.
- [`math/`](math/): 28 formal LaTeX academic papers detailing relativistic physics algorithms and DSP stability proofs.

- [`docs/CONCEPT_AND_COMPOSITE_NODES.md`](docs/CONCEPT_AND_COMPOSITE_NODES.md): Relativistic time effects on audio/control objects and composite sub-graph (`[patch~]`) architecture for building complex objects from smaller sub-nodes.
