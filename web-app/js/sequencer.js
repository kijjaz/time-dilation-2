// Connect Step Sequencers to Universal Temporal Bus
class StepSequencer {
  constructor(engine, bus) {
    this.engine = engine;
    this.bus = bus;
    this.currentStep = 0;
    this.totalSteps = 16;
    this.timer = null;
    this.bpm = 120;

    this.currentScale = 'minor';

    this.grid = {
      drums: {
        kick:  [1, 0, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0],
        snare: [0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0],
        hihat: [1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1],
        perc:  [0, 0, 1, 0,  0, 0, 1, 0,  0, 0, 1, 0,  0, 1, 0, 0]
      },
      bass: [
        { active: true, note: 'C2' }, { active: false, note: 'C2' }, { active: true, note: 'C2' }, { active: false, note: 'C2' },
        { active: true, note: 'D2' }, { active: false, note: 'D2' }, { active: true, note: 'Eb2' }, { active: false, note: 'Eb2' },
        { active: true, note: 'F2' }, { active: false, note: 'F2' }, { active: true, note: 'G2' }, { active: false, note: 'G2' },
        { active: true, note: 'Bb2' }, { active: false, note: 'Bb2' }, { active: true, note: 'C2' }, { active: false, note: 'C2' }
      ],
      lead: [
        { active: true, note: 'C4' }, { active: true, note: 'Eb4' }, { active: true, note: 'G4' }, { active: true, note: 'Bb4' },
        { active: true, note: 'C5' }, { active: true, note: 'Bb4' }, { active: true, note: 'G4' }, { active: true, note: 'Eb4' },
        { active: true, note: 'D4' }, { active: true, note: 'F4' }, { active: true, note: 'A4' }, { active: true, note: 'C5' },
        { active: true, note: 'Bb4' }, { active: true, note: 'G4' }, { active: true, note: 'F4' }, { active: true, note: 'D4' }
      ]
    };
  }

  setScale(scaleName) {
    if (this.engine.scales[scaleName]) {
      this.currentScale = scaleName;
    }
  }

  setBassNote(step, note) { this.grid.bass[step].note = note; }
  setLeadNote(step, note) { this.grid.lead[step].note = note; }

  toggleDrumStep(sound, step) { this.grid.drums[sound][step] = this.grid.drums[sound][step] ? 0 : 1; }
  toggleBassStep(step) { this.grid.bass[step].active = !this.grid.bass[step].active; }
  toggleLeadStep(step) { this.grid.lead[step].active = !this.grid.lead[step].active; }

  start() {
    this.stop();
    this.currentStep = 0;
    this.tick();
  }

  stop() {
    if (this.timer) {
      clearTimeout(this.timer);
      this.timer = null;
    }
  }

  tick() {
    if (!this.engine || !this.engine.isPlaying) return;

    this.triggerStep(this.currentStep);

    if (window.highlightSequencerStep) {
      window.highlightSequencerStep(this.currentStep);
    }

    // Effective Speed & Phase Offset from Temporal Bus
    const speed = this.bus ? this.bus.getEffectiveSpeed('drumsSeq') : 1.0;
    const offsetMs = this.bus ? this.bus.getEffectiveOffset('drumsSeq') : 0;
    const baseStepDurationMs = ((60 / this.bpm) / 4 * 1000) / Math.max(0.1, speed);
    const finalDurationMs = Math.max(10, baseStepDurationMs + offsetMs);

    this.currentStep = (this.currentStep + 1) % this.totalSteps;
    this.timer = setTimeout(() => this.tick(), finalDurationMs);
  }

  triggerStep(step) {
    if (!this.engine.isStarted) return;

    // Drums
    Object.keys(this.grid.drums).forEach(sound => {
      if (this.grid.drums[sound][step]) {
        this.engine.triggerDrum(sound);
      }
    });

    // Bass
    const bassStep = this.grid.bass[step];
    if (bassStep.active) {
      this.engine.triggerBass(bassStep.note);
    }

    // Lead
    const leadStep = this.grid.lead[step];
    if (leadStep.active) {
      this.engine.triggerLead(leadStep.note);
    }
  }
}

window.StepSequencer = StepSequencer;
