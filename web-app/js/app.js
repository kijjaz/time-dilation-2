// Main App Controller - Binding Universal Temporal Bus to UI & Modules
document.addEventListener('DOMContentLoaded', () => {
  const temporalBus = new window.TemporalBus();
  const engine = window.timeEngine;
  const matrix = new window.RoutingMatrix(engine);
  const sequencer = new window.StepSequencer(engine, temporalBus);
  const visualizer = new window.Visualizer('vis-canvas', engine);

  // UI Controls
  const btnStartAudio = document.getElementById('btn-audio-engine');
  const engineStatus = document.getElementById('engine-status');
  const footStatus = document.getElementById('foot-status');
  const footCtx = document.getElementById('foot-ctx');

  const dilationSlider = document.getElementById('dilation-slider');
  const dilationValueDisplay = document.getElementById('dilation-value');
  const perceivedBpmDisplay = document.getElementById('perceived-bpm');
  const semitoneShiftDisplay = document.getElementById('semitone-shift');

  const offsetSlider = document.getElementById('offset-slider');
  const offsetValDisplay = document.getElementById('offset-val');

  const btnPlay = document.getElementById('btn-play');
  const btnStop = document.getElementById('btn-stop');
  const btnRec = document.getElementById('btn-rec');
  const btnExportJson = document.getElementById('btn-export-json');
  const scaleSelect = document.getElementById('scale-select');

  buildStepGrids(sequencer);

  // Audio Engine Start
  btnStartAudio.addEventListener('click', async () => {
    await engine.init(temporalBus);
    engineStatus.textContent = 'ONLINE (48kHz)';
    engineStatus.classList.add('online');
    footStatus.textContent = 'RUNNING';
    footCtx.textContent = `ACTIVE (${engine.ctx.sampleRate / 1000} kHz)`;
    btnStartAudio.textContent = '⚡ ENGINE ACTIVE';
    btnStartAudio.style.opacity = '0.7';
  });

  // Universal Speed Slider
  dilationSlider.addEventListener('input', (e) => {
    const val = parseFloat(e.target.value);
    temporalBus.setSpeed(val);
  });

  // Universal Phase Offset Slider
  offsetSlider.addEventListener('input', (e) => {
    const val = parseFloat(e.target.value);
    offsetValDisplay.textContent = `${val} ms`;
    temporalBus.setOffset(val);
  });

  // Target Checkboxes
  document.querySelectorAll('.target-item input').forEach(checkbox => {
    checkbox.addEventListener('change', (e) => {
      const targetKey = e.target.getAttribute('data-target');
      temporalBus.setTargetState(targetKey, e.target.checked);
    });
  });

  // Global Readout Updates from Temporal Bus
  window.updateDilationReadouts = (gamma, bpm, semitones) => {
    dilationValueDisplay.innerHTML = `${gamma.toFixed(3)}&times;`;
    perceivedBpmDisplay.textContent = `${bpm} BPM`;
    const sign = semitones >= 0 ? '+' : '';
    semitoneShiftDisplay.textContent = `${sign}${semitones.toFixed(2)}st`;
  };

  // Sound Tweakers
  document.getElementById('bass-wave').addEventListener('change', (e) => { engine.params.bass.wave = e.target.value; });
  document.getElementById('bass-cutoff').addEventListener('input', (e) => { engine.params.bass.cutoff = parseFloat(e.target.value); });
  document.getElementById('lead-wave').addEventListener('change', (e) => { engine.params.lead.wave = e.target.value; });

  scaleSelect.addEventListener('change', (e) => {
    sequencer.setScale(e.target.value);
    buildStepGrids(sequencer);
  });

  // Signal Matrix Routing
  document.querySelectorAll('.matrix-select').forEach(select => {
    select.addEventListener('change', (e) => {
      const voice = e.target.getAttribute('data-voice');
      matrix.setRoute(voice, e.target.value);
    });
  });

  // Transport & Rec
  btnPlay.addEventListener('click', async () => {
    if (!engine.isStarted) await engine.init(temporalBus);
    engine.isPlaying = true;
    sequencer.start();
    btnPlay.classList.add('active');
  });

  btnStop.addEventListener('click', () => {
    engine.isPlaying = false;
    sequencer.stop();
    btnPlay.classList.remove('active');
    if (engine.isRecording) {
      engine.stopRecordingAndDownload();
      btnRec.classList.remove('recording');
      btnRec.textContent = '🔴 RECORD WAV';
    }
  });

  btnRec.addEventListener('click', async () => {
    if (!engine.isStarted) await engine.init(temporalBus);
    if (!engine.isRecording) {
      engine.startRecording();
      btnRec.classList.add('recording');
      btnRec.textContent = '⏹ STOP & SAVE WAV';
    } else {
      engine.stopRecordingAndDownload();
      btnRec.classList.remove('recording');
      btnRec.textContent = '🔴 RECORD WAV';
    }
  });

  btnExportJson.addEventListener('click', () => {
    const songData = {
      version: '4.0',
      temporalBus: { speed: temporalBus.speed, offset: temporalBus.offset, targets: temporalBus.targets },
      routes: matrix.routes,
      params: engine.params,
      grid: sequencer.grid
    };
    const blob = new Blob([JSON.stringify(songData, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `time-dilation-universal-${Date.now()}.json`;
    a.click();
  });

  window.highlightSequencerStep = (step) => {
    document.querySelectorAll('.step-cell').forEach(cell => cell.classList.remove('current-step'));
    document.querySelectorAll(`.step-cell[data-step="${step}"]`).forEach(cell => cell.classList.add('current-step'));
  };
});

function buildStepGrids(sequencer) {
  const engine = window.timeEngine;
  const currentScaleNotes = engine.scales[sequencer.currentScale] || engine.scales.minor;

  ['kick', 'snare', 'hihat', 'perc'].forEach(sound => {
    const container = document.querySelector(`.step-cells[data-seq="drums"][data-sound="${sound}"]`);
    if (!container) return;
    container.innerHTML = '';

    for (let i = 0; i < 16; i++) {
      const cell = document.createElement('div');
      cell.className = 'step-cell';
      cell.setAttribute('data-step', i);
      if (sequencer.grid.drums[sound][i]) cell.classList.add('active');

      cell.addEventListener('click', () => {
        sequencer.toggleDrumStep(sound, i);
        cell.classList.toggle('active');
      });
      container.appendChild(cell);
    }
  });

  const bassContainer = document.querySelector('.step-cells[data-seq="bass"]');
  if (bassContainer) {
    bassContainer.innerHTML = '';
    for (let i = 0; i < 16; i++) {
      const cell = document.createElement('div');
      cell.className = 'step-cell';
      cell.setAttribute('data-step', i);
      if (sequencer.grid.bass[i].active) cell.classList.add('active');

      const select = document.createElement('select');
      select.className = 'note-select-cell';
      currentScaleNotes.slice(0, 8).forEach(note => {
        const opt = document.createElement('option');
        opt.value = note;
        opt.textContent = note;
        if (note === sequencer.grid.bass[i].note) opt.selected = true;
        select.appendChild(opt);
      });

      select.addEventListener('change', (e) => {
        sequencer.setBassNote(i, e.target.value);
      });

      cell.addEventListener('click', (e) => {
        if (e.target !== select) {
          sequencer.toggleBassStep(i);
          cell.classList.toggle('active');
        }
      });

      cell.appendChild(select);
      bassContainer.appendChild(cell);
    }
  }

  const leadContainer = document.querySelector('.step-cells[data-seq="lead"]');
  if (leadContainer) {
    leadContainer.innerHTML = '';
    for (let i = 0; i < 16; i++) {
      const cell = document.createElement('div');
      cell.className = 'step-cell';
      cell.setAttribute('data-step', i);
      if (sequencer.grid.lead[i].active) cell.classList.add('active');

      const select = document.createElement('select');
      select.className = 'note-select-cell';
      currentScaleNotes.slice(4).forEach(note => {
        const opt = document.createElement('option');
        opt.value = note;
        opt.textContent = note;
        if (note === sequencer.grid.lead[i].note) opt.selected = true;
        select.appendChild(opt);
      });

      select.addEventListener('change', (e) => {
        sequencer.setLeadNote(i, e.target.value);
      });

      cell.addEventListener('click', (e) => {
        if (e.target !== select) {
          sequencer.toggleLeadStep(i);
          cell.classList.toggle('active');
        }
      });

      cell.appendChild(select);
      leadContainer.appendChild(cell);
    }
  }
}
