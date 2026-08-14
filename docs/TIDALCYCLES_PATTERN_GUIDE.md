# TidalCycles Pattern & Mini-Notation User Guide

`Time Dilation DAW 2` features a full-fledged **TidalCycles Mini-Notation Pattern Engine** directly integrated with relativistic proper-time clocks ($\tau, \gamma$).

---

## 1. Syntax Quick Reference

### 1.1 Nested Subdivisions (`[a b c]`)
Enclose items in square brackets `[ ... ]` to subdivide a cycle or step evenly among them:
```tidal
[60 62 64 67]               -- 4 equal quarter notes per cycle
[60 [62 64] 67 [69 71 72]]  -- Step 1 (1/4), Step 2 (two 1/8ths), Step 3 (1/4), Step 4 (triplet 1/12ths)
[[60 62] [64 [65 67]]]      -- Deeply nested micro-divisions
```

### 1.2 Polyphonic Stacking (`[melody, bass, drums]`)
Use commas `,` to run multiple independent rhythmic patterns simultaneously in the same cycle:
```tidal
[60 [62 65] 67, 36 [~ 48]]         -- Channel 0 plays lead melody; Channel 1 plays bassline
[60 64 67, 36 [~ 48], 42*4]        -- Lead + Bass + Hi-hats in one unified pattern
```

### 1.3 Embedded Euclidean Notation (`pitch(k, n, rot)`)
Embed Bjorklund Euclidean pulse distributions directly onto any pitch or pattern token:
```tidal
60(3,8)            -- 3 pulses distributed across 8 steps: [X . . X . . X .] (Tresillo)
36(5,16,2)         -- 5 pulses distributed across 16 steps rotated by 2 steps: Cinquillo
[36(3,8), 42(7,16)]-- Poly-Euclidean groove (3 over 8 against 7 over 16)
```

### 1.4 Speed Multipliers (`*n`, `/n`)
Speed up or slow down specific elements within a step:
```tidal
60*4               -- Plays note 60 four times as fast inside its allocated step slot
[60 62]*2          -- Repeats the sequence [60 62] twice inside its slot
[60 64 67]/2       -- Plays the sequence at half speed (spread over 2 cycles)
```

### 1.5 Cycle Alternation (`<a b c>`)
Angle brackets `< ... >` cycle through items across successive bars/cycles:
```tidal
<60 62 65 67>            -- Plays note 60 in bar 1, 62 in bar 2, 65 in bar 3, 67 in bar 4
[<60 62> <64 65> 67 72]  -- Alternates chords/melodies per measure
```

### 1.6 Rests & Probability (`~`, `?prob`)
- `~`: Silent rest step (no note/trigger emitted).
- `?prob`: Stochastic probability (e.g. `?0.8` = 80% chance to trigger; `?` defaults to 50%).
```tidal
[60 ~ 64 ~]         -- Alternating note and rest
[60?0.9 62?0.5 ~]   -- Probabilistic generative triggers
```

### 1.7 Drum Synth Aliases
Use standard drum aliases or MIDI note numbers (0–127):
| Alias | Instrument | MIDI Note |
| :--- | :--- | :--- |
| `bd` / `kick` | Bass Drum / Kick | 36 |
| `sn` / `snare` | Snare Drum | 38 |
| `cp` / `clap` | Hand Clap | 39 |
| `hh` / `hat` | Closed Hi-Hat | 42 |
| `oh` | Open Hi-Hat | 46 |
| `lt` / `mt` / `ht` | Low / Mid / High Tom | 41 / 45 / 48 |
| `cb` | Cowbell | 56 |
| `cl` / `rim` | Claves / Rimshot | 75 / 37 |

---

## 2. Where to Access in the App

1. **Top Menu**:
   - Go to **`Help -> TidalCycles Mini-Notation & Pattern Guide`** to open the full in-app reference modal.
2. **Arrangement Timeline Drawer**:
   - Open the **TIDAL DRAWER** at the bottom of the timeline and click the cyan **`[?] HELP`** button on the right side of the toolbar.
   - Use the 1-click transformation macros: `[a b] /2`, `[a b c] /3`, `+ Stack Poly (,)`, `Euclid (3,8)`, `<a b> Alt`, `*2 Speed`, `? Degrade`.
3. **Terminal Console (`Cmd+K`)**:
   - Type `help tidal` or `help seq.tidal` to view the CLI cheatsheet.
4. **Node Inspector**:
   - Click any `seq.tidal`, `tidal`, or `pattern` node on the canvas to view parameters, cycle duration sliders, and instant clickable pattern templates.
5. **In-App Preset**:
   - Go to **`Help -> Examples & Presets -> 07: TidalCycles Relativistic Nested Polyphony Rig`** for an immediate live demonstration.
