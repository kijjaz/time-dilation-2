# Relativistic Physics & Mathematics Reference Guide

> **Project**: Time Dilation DAW (v0.0.3 Architecture)  
> **Location**: `/Users/kijjaz/Desktop/Antigravity/2026/20260812 Time Dilation DAW 2/docs/MATH_THEORY_GUIDE.md`  

---

## 1. Local Coordinate Time Dilation ($\gamma$)

Every node operates on its own local proper coordinate time clock $\tau$. The speed ratio relative to global real-time master clock $t$ is:

$$\gamma = \frac{d\tau}{dt}$$

For discrete digital processing across buffer block size $N$ samples at sample rate $f_s$:

$$dt = \frac{N}{f_s}$$
$$\Delta \tau = \gamma \cdot dt = \gamma \cdot \frac{N}{f_s}$$

### Operational States of Gamma ($\gamma$):
- **Standard Real-Time ($\gamma = 1.0$)**: Normal playback speed and standard physical pitch.
- **Time Warp Speed ($\gamma > 1.0$)**: Accelerated time flow. Frequency Doppler shifts upwards ($f = f_{\text{base}} \cdot \gamma$).
- **Time Slowdown ($0.0 < \gamma < 1.0$)**: Slow motion time flow. Frequency Doppler shifts downwards.
- **Gravitational Freeze / Stasis ($\gamma = 0.0$)**: Time halts ($\Delta \tau = 0$). Signal phase holds instantaneous voltage value.
- **Temporal Reversal / Retrograde ($\gamma < 0.0$)**: Time flows backwards ($\Delta \tau < 0$). Waveform phase steps in reverse through memory buffers.

---

## 2. Relativistic Velocity Addition Composition (Lorentz Transformation)

When combining two cascading time dilation nodes (e.g. `time.warp~` $\gamma_1$ into `time.warp~` $\gamma_2$), speed addition obeys the **Relativistic Lorentz Velocity Addition Theorem** normalized to speed of light $c = 1.0$:

$$\gamma_{\text{combined}} = \frac{\gamma_1 + \gamma_2}{1 + \gamma_1 \cdot \gamma_2}$$

### C++ DSP Math:
```cpp
double combineSpeeds (double gamma1, double gamma2)
{
    double denom = 1.0 + (gamma1 * gamma2);
    if (std::abs(denom) < 1e-9) return 0.0; // Avoid singularity
    return (gamma1 + gamma2) / denom;
}
```

---

## 3. Polyphonic Time Stream Cloud Math (`TimePolyFrame`)

A granular time cloud (`time.grain~`) fans 1 master stream into $K$ polyphonic micro-time streams ($\tau_1, \tau_2, \dots, \tau_K$).

Each micro-stream $k$ has its own velocity, offset, and windowing envelope:

$$\text{Stream } k: \quad \tau_k(t) = \tau_0 + \Delta \tau_k + \int_0^t \gamma_k(s) \, ds$$

Envelope Windowing (Hann / Sine Window based on normalized age ratio):

$$A_k(t) = \sin\left(\pi \cdot \frac{\text{age}_k}{\text{duration}_k}\right), \quad 0 \le \text{age}_k \le \text{duration}_k$$

Total output signal $y[n]$ from oscillator or sampler summing over active streams:

$$y[n] = \sum_{k=1}^{K} A_k[n] \cdot S\left(f_s \cdot \tau_k[n]\right)$$

---

## 4. Fractional Resampling with 4-Point C1 Cubic Hermite Interpolation

To prevent aliasing and digital stair-stepping when coordinate time dilation $\gamma$ varies continuously, audio and time values are sampled from tables using 4-point C1 Cubic Hermite Polynomial Resamplers:

Given fractional table index position $p = i + f$ (where $i = \lfloor p \rfloor$ and $f = p - i$), sample points $y_{-1}, y_0, y_1, y_2$:

$$c_0 = y_0$$
$$c_1 = \frac{1}{2}(y_1 - y_{-1})$$
$$c_2 = y_{-1} - \frac{5}{2}y_0 + 2y_1 - \frac{1}{2}y_2$$
$$c_3 = \frac{1}{2}(y_2 - y_{-1}) + \frac{3}{2}(y_0 - y_1)$$

$$\text{Interpolated Value: } y(p) = ((c_3 \cdot f + c_2) \cdot f + c_1) \cdot f + c_0$$

---

## 5. Topological Feedback Loop Stability (Tarjan SCC & 1-Block History)

When patch cables form a closed feedback loop:

$$\text{Node A} \longrightarrow \text{Node B} \longrightarrow \text{Node C} \longrightarrow \text{Node A}$$

Direct instantaneous evaluation causes deadlocks or sample rate crashes.

### Stability Proof (1-Block History Delay):
By inserting a 1-block delay buffer $z^{-1}$ on destination ports marked as cycle feedback paths:

$$x_{\text{dest}}[n] = x_{\text{src, previousBlockBuffer}}[n - N]$$

This decouples the cyclic dependency, guaranteeing 100% deterministic execution order in $O(V + E)$ time without sample rate crashes.

---

## 6. Chamberlin State-Variable Filter (SVF) Mathematics

`svf~` implements the zero-delay feedback (ZDF) Chamberlin State-Variable Filter:

Given cutoff frequency $f_c$ and resonance quality $Q$:

$$g = \tan\left(\frac{\pi f_c}{f_s}\right), \quad k = \frac{1}{\max(0.1, Q)}$$

Per-sample state update:

$$v_3 = x[n] - s_2$$
$$v_1 = \frac{s_1 + g \cdot v_3}{1 + g(g + k)}$$
$$v_2 = s_2 + g \cdot v_1$$

$$s_1 \leftarrow 2 v_1 - s_1, \quad s_2 \leftarrow 2 v_2 - s_2$$

Outlets:
- **Lowpass ($y_{\text{LP}}$)**: $v_2$
- **Highpass ($y_{\text{HP}}$)**: $x[n] - k v_1 - v_2$
- **Bandpass ($y_{\text{BP}}$)**: $v_1$
- **Notch ($y_{\text{BR}}$)**: $y_{\text{HP}} + y_{\text{LP}}$
