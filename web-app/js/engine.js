// Connect Web Audio Engine FX & Synthesizers to Universal Temporal Bus
class TimeDilationEngine {
  constructor() {
    this.ctx = null;
    this.masterGain = null;
    this.analyser = null;
    this.isStarted = false;
    this.isPlaying = false;
    
    this.temporalBus = null; // Bound on init

    this.isRecording = false;
    this.audioChunks = [];
    this.mediaRecorder = null;

    this.params = {
      drums: { pitch: 1.0, decay: 0.15 },
      bass: { wave: 'sawtooth', cutoff: 600, res: 4.0, attack: 0.01, decay: 0.25 },
      lead: { wave: 'square', cutoff: 2000, res: 2.0, attack: 0.01, decay: 0.2, detune: 5 }
    };

    this.customSamples = { kick: null, snare: null, hihat: null, perc: null };
    this.fxNodes = {};
    this.voices = { drums: null, bass: null, lead: null };

    this.scales = {
      minor: ['C2', 'Eb2', 'F2', 'G2', 'Bb2', 'C3', 'Eb3', 'F3', 'G3', 'Bb3', 'C4', 'Eb4', 'F4', 'G4', 'Bb4', 'C5'],
      major: ['C2', 'D2', 'E2', 'F2', 'G2', 'A2', 'B2', 'C3', 'D3', 'E3', 'F3', 'G3', 'A3', 'B3', 'C4', 'D4'],
      pentatonic: ['C2', 'Eb2', 'F2', 'G2', 'Bb2', 'C3', 'Eb3', 'F3', 'G3', 'Bb3', 'C4', 'Eb4', 'F4', 'G4', 'Bb4']
    };

    this.noteFreqs = {
      'C2': 65.41, 'D2': 73.42, 'Eb2': 77.78, 'E2': 82.41, 'F2': 87.31, 'G2': 98.00, 'A2': 110.00, 'B2': 123.47, 'Bb2': 116.54,
      'C3': 130.81, 'Eb3': 155.56, 'F3': 174.61, 'G3': 196.00, 'Bb3': 233.08,
      'C4': 261.63, 'D4': 293.66, 'Eb4': 311.13, 'E4': 329.63, 'F4': 349.23, 'G4': 392.00, 'A4': 440.00, 'Bb4': 466.16, 'B4': 493.88, 'C5': 523.25
    };
  }

  async init(bus) {
    if (this.isStarted) return;
    this.temporalBus = bus;

    const AudioContext = window.AudioContext || window.webkitAudioContext;
    this.ctx = new AudioContext({ sampleRate: 48000 });

    this.masterGain = this.ctx.createGain();
    this.masterGain.gain.value = 0.85;

    this.analyser = this.ctx.createAnalyser();
    this.analyser.fftSize = 1024;

    this.setupFXNodes();
    this.setupVoiceNodes();

    this.masterGain.connect(this.analyser);
    this.analyser.connect(this.ctx.destination);

    this.recordDest = this.ctx.createMediaStreamDestination();
    this.masterGain.connect(this.recordDest);
    this.mediaRecorder = new MediaRecorder(this.recordDest.stream);
    this.mediaRecorder.ondataavailable = (e) => {
      if (e.data.size > 0) this.audioChunks.push(e.data);
    };

    // Subscribe to Universal Temporal Bus updates
    if (this.temporalBus) {
      this.temporalBus.subscribe(() => this.onTemporalUpdate());
    }

    this.isStarted = true;
    if (this.ctx.state === 'suspended') await this.ctx.resume();
  }

  onTemporalUpdate() {
    if (!this.isStarted || !this.temporalBus) return;

    // 1. Update Echo Delay Time via Universal Bus
    const delaySpeed = this.temporalBus.getEffectiveSpeed('echoDelay');
    const delayOffsetMs = this.temporalBus.getEffectiveOffset('echoDelay');
    if (this.fxNodes.delay) {
      const baseDelay = 0.25 / Math.max(0.1, delaySpeed);
      this.fxNodes.delay.delayTime.value = Math.min(2.0, baseDelay + (delayOffsetMs / 1000.0));
    }

    // 2. Update Lowpass Filter LFO Cutoff via Universal Bus
    const filterSpeed = this.temporalBus.getEffectiveSpeed('filterLfo');
    if (this.fxNodes.filter) {
      this.fxNodes.filter.frequency.value = Math.min(8000, 1200 * filterSpeed);
    }
  }

  setupFXNodes() {
    this.fxNodes.direct = this.masterGain;

    const filter = this.ctx.createBiquadFilter();
    filter.type = 'lowpass';
    filter.frequency.value = 1200;
    filter.Q.value = 5.0;
    filter.connect(this.masterGain);
    this.fxNodes.filter = filter;

    const delay = this.ctx.createDelay();
    delay.delayTime.value = 0.25;
    const delayFeedback = this.ctx.createGain();
    delayFeedback.gain.value = 0.4;
    delay.connect(delayFeedback);
    delayFeedback.connect(delay);
    delay.connect(this.masterGain);
    this.fxNodes.delay = delay;

    const warpGain = this.ctx.createGain();
    warpGain.gain.value = 0.9;
    warpGain.connect(this.masterGain);
    this.fxNodes.warp = warpGain;
  }

  setupVoiceNodes() {
    ['drums', 'bass', 'lead'].forEach(v => {
      const g = this.ctx.createGain();
      g.gain.value = 0.8;
      g.connect(this.fxNodes.direct);
      this.voices[v] = g;
    });
  }

  updateVoiceRouting(voiceId, fxTarget) {
    if (!this.voices[voiceId]) return;
    const voiceNode = this.voices[voiceId];
    voiceNode.disconnect();
    const targetNode = this.fxNodes[fxTarget] || this.fxNodes.direct;
    voiceNode.connect(targetNode);
  }

  // --- Triggers ---
  triggerDrum(type) {
    if (!this.isStarted) return;
    const now = this.ctx.currentTime;
    const out = this.voices.drums;
    const p = this.params.drums;
    const drumSpeed = this.temporalBus ? this.temporalBus.getEffectiveSpeed('drumsSeq') : 1.0;

    if (type === 'kick') {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      osc.frequency.setValueAtTime(130 * p.pitch * drumSpeed, now);
      osc.frequency.exponentialRampToValueAtTime(30 * p.pitch * drumSpeed, now + p.decay);
      gain.gain.setValueAtTime(1.0, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + p.decay);
      osc.connect(gain);
      gain.connect(out);
      osc.start(now);
      osc.stop(now + p.decay);
    } else if (type === 'snare') {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      osc.type = 'triangle';
      osc.frequency.setValueAtTime(220 * p.pitch * drumSpeed, now);
      gain.gain.setValueAtTime(0.7, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + p.decay);
      osc.connect(gain);
      gain.connect(out);
      osc.start(now);
      osc.stop(now + p.decay);
    } else if (type === 'hihat') {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      osc.type = 'square';
      osc.frequency.setValueAtTime(7000 * p.pitch * drumSpeed, now);
      gain.gain.setValueAtTime(0.3, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + (p.decay * 0.4));
      osc.connect(gain);
      gain.connect(out);
      osc.start(now);
      osc.stop(now + p.decay * 0.4);
    } else if (type === 'perc') {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      osc.type = 'sine';
      osc.frequency.setValueAtTime(480 * p.pitch * drumSpeed, now);
      gain.gain.setValueAtTime(0.5, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + p.decay);
      osc.connect(gain);
      gain.connect(out);
      osc.start(now);
      osc.stop(now + p.decay);
    }
  }

  triggerBass(note) {
    if (!this.isStarted) return;
    const now = this.ctx.currentTime;
    const p = this.params.bass;
    const bassSpeed = this.temporalBus ? this.temporalBus.getEffectiveSpeed('bassSeq') : 1.0;
    const freq = (this.noteFreqs[note] || 65.41) * bassSpeed;
    const out = this.voices.bass;

    const osc = this.ctx.createOscillator();
    const filter = this.ctx.createBiquadFilter();
    const gain = this.ctx.createGain();

    osc.type = p.wave;
    osc.frequency.setValueAtTime(freq, now);

    filter.type = 'lowpass';
    filter.frequency.setValueAtTime(p.cutoff * bassSpeed, now);
    filter.Q.value = p.res;

    gain.gain.setValueAtTime(0.001, now);
    gain.gain.linearRampToValueAtTime(0.7, now + p.attack);
    gain.gain.exponentialRampToValueAtTime(0.001, now + p.attack + p.decay);

    osc.connect(filter);
    filter.connect(gain);
    gain.connect(out);

    osc.start(now);
    osc.stop(now + p.attack + p.decay);
  }

  triggerLead(note) {
    if (!this.isStarted) return;
    const now = this.ctx.currentTime;
    const p = this.params.lead;
    const leadSpeed = this.temporalBus ? this.temporalBus.getEffectiveSpeed('leadSeq') : 1.0;
    const freq = (this.noteFreqs[note] || 440) * leadSpeed;
    const out = this.voices.lead;

    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = p.wave;
    osc.frequency.setValueAtTime(freq, now);

    gain.gain.setValueAtTime(0.001, now);
    gain.gain.linearRampToValueAtTime(0.4, now + p.attack);
    gain.gain.exponentialRampToValueAtTime(0.001, now + p.attack + p.decay);

    osc.connect(gain);
    gain.connect(out);

    osc.start(now);
    osc.stop(now + p.attack + p.decay);
  }

  startRecording() {
    if (!this.mediaRecorder) return;
    this.audioChunks = [];
    this.mediaRecorder.start();
    this.isRecording = true;
  }

  stopRecordingAndDownload() {
    if (!this.mediaRecorder || !this.isRecording) return;
    this.mediaRecorder.stop();
    this.isRecording = false;

    this.mediaRecorder.onstop = () => {
      const blob = new Blob(this.audioChunks, { type: 'audio/wav' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.style.display = 'none';
      a.href = url;
      a.download = `time-dilation-recording-${Date.now()}.wav`;
      document.body.appendChild(a);
      a.click();
      setTimeout(() => URL.revokeObjectURL(url), 100);
    };
  }
}

window.timeEngine = new TimeDilationEngine();
