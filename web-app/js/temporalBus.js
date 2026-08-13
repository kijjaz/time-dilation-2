// Universal Temporal Modulation Bus
// Manages Time Speed (Gamma) and Temporal Phase Offset (Delta Phi) across all subscribed modules.
class TemporalBus {
  constructor() {
    this.speed = 1.0;     // Time Speed Factor (0.10x to 4.00x)
    this.offset = 0;      // Temporal Offset Shift in milliseconds (0 to 1000ms)
    
    // Subscribed Modules Map & Target Modulation Enables
    this.targets = {
      drumsSeq:  { enabled: true, speedRatio: 1.0, offsetMs: 0 },
      bassSeq:   { enabled: true, speedRatio: 1.0, offsetMs: 0 },
      leadSeq:   { enabled: true, speedRatio: 1.0, offsetMs: 0 },
      filterLfo: { enabled: true, speedRatio: 1.0, offsetMs: 0 },
      echoDelay: { enabled: true, speedRatio: 1.0, offsetMs: 0 },
      visualizer:{ enabled: true, speedRatio: 1.0, offsetMs: 0 }
    };

    this.subscribers = [];
  }

  setSpeed(val) {
    this.speed = parseFloat(val);
    this.notify();
  }

  setOffset(val) {
    this.offset = parseFloat(val);
    this.notify();
  }

  setTargetState(targetKey, enabled, speedRatio = 1.0, offsetMs = 0) {
    if (this.targets[targetKey]) {
      this.targets[targetKey].enabled = enabled;
      this.targets[targetKey].speedRatio = parseFloat(speedRatio);
      this.targets[targetKey].offsetMs = parseFloat(offsetMs);
      this.notify();
    }
  }

  getEffectiveSpeed(targetKey) {
    const t = this.targets[targetKey];
    if (!t || !t.enabled) return 1.0;
    return this.speed * t.speedRatio;
  }

  getEffectiveOffset(targetKey) {
    const t = this.targets[targetKey];
    if (!t || !t.enabled) return 0;
    return this.offset + t.offsetMs;
  }

  subscribe(callback) {
    this.subscribers.push(callback);
  }

  notify() {
    this.subscribers.forEach(cb => cb(this));
  }
}

window.TemporalBus = TemporalBus;
