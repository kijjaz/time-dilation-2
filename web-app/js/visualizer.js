// Visualizer module - Real-Time Gravitational Time-Warp Oscilloscope & Spectrogram
class Visualizer {
  constructor(canvasId, engine) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext('2d');
    this.engine = engine;
    this.mode = 'oscilloscope'; // 'oscilloscope' or 'spectrogram'
    this.animId = null;

    this.init();
  }

  init() {
    this.resize();
    window.addEventListener('resize', () => this.resize());
    this.draw();
  }

  resize() {
    if (this.canvas) {
      this.canvas.width = this.canvas.clientWidth * window.devicePixelRatio;
      this.canvas.height = this.canvas.clientHeight * window.devicePixelRatio;
    }
  }

  toggleMode() {
    this.mode = this.mode === 'oscilloscope' ? 'spectrogram' : 'oscilloscope';
    return this.mode;
  }

  draw() {
    this.animId = requestAnimationFrame(() => this.draw());

    const width = this.canvas.width;
    const height = this.canvas.height;
    this.ctx.fillStyle = '#08090c';
    this.ctx.fillRect(0, 0, width, height);

    // Draw Relativistic Background Warp Grid
    this.drawTimeWarpGrid(width, height);

    if (!this.engine || !this.engine.analyser) {
      this.drawIdleLine(width, height);
      return;
    }

    const bufferLength = this.engine.analyser.frequencyBinCount;
    const dataArray = new Uint8Array(bufferLength);

    if (this.mode === 'oscilloscope') {
      this.engine.analyser.getByteTimeDomainData(dataArray);
      this.drawOscilloscope(dataArray, bufferLength, width, height);
    } else {
      this.engine.analyser.getByteFrequencyData(dataArray);
      this.drawSpectrogram(dataArray, bufferLength, width, height);
    }
  }

  drawTimeWarpGrid(w, h) {
    const dilation = this.engine ? this.engine.dilationFactor : 1.0;
    this.ctx.strokeStyle = 'rgba(212, 175, 55, 0.08)';
    this.ctx.lineWidth = 1;

    // Draw distorted vertical time grid lines based on gamma factor
    const cols = 16;
    for (let i = 0; i <= cols; i++) {
      const normX = i / cols;
      // Gravitational lens curvature formula
      const warpedX = Math.pow(normX, 1.0 / Math.max(0.2, dilation)) * w;

      this.ctx.beginPath();
      this.ctx.moveTo(warpedX, 0);
      this.ctx.lineTo(warpedX, h);
      this.ctx.stroke();
    }
  }

  drawIdleLine(w, h) {
    this.ctx.strokeStyle = '#d4af37';
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    this.ctx.moveTo(0, h / 2);
    this.ctx.lineTo(w, h / 2);
    this.ctx.stroke();
  }

  drawOscilloscope(dataArray, bufferLength, w, h) {
    this.ctx.lineWidth = 2.5;
    this.ctx.strokeStyle = '#00f0ff';
    this.ctx.shadowBlur = 10;
    this.ctx.shadowColor = 'rgba(0, 240, 255, 0.5)';

    this.ctx.beginPath();
    const sliceWidth = w / bufferLength;
    let x = 0;

    for (let i = 0; i < bufferLength; i++) {
      const v = dataArray[i] / 128.0;
      const y = (v * h) / 2;

      if (i === 0) {
        this.ctx.moveTo(x, y);
      } else {
        this.ctx.lineTo(x, y);
      }
      x += sliceWidth;
    }

    this.ctx.stroke();
    this.ctx.shadowBlur = 0;
  }

  drawSpectrogram(dataArray, bufferLength, w, h) {
    const barWidth = (w / bufferLength) * 2.5;
    let x = 0;

    for (let i = 0; i < bufferLength; i++) {
      const barHeight = (dataArray[i] / 255) * h;

      const gradient = this.ctx.createLinearGradient(0, h, 0, h - barHeight);
      gradient.addColorStop(0, '#d4af37');
      gradient.addColorStop(1, '#ff4757');

      this.ctx.fillStyle = gradient;
      this.ctx.fillRect(x, h - barHeight, barWidth, barHeight);

      x += barWidth + 1;
    }
  }
}

window.Visualizer = Visualizer;
