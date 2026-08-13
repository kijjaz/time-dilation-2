// Signal Routing Matrix Manager
class RoutingMatrix {
  constructor(engine) {
    this.engine = engine;
    
    // Default Routing Chains for Voices
    // Voices: 'drums', 'bass', 'lead'
    // Available FX Nodes: 'direct', 'filter', 'delay', 'warp'
    this.routes = {
      drums: 'direct',
      bass: 'filter',
      lead: 'delay'
    };
  }

  setRoute(voiceId, fxTarget) {
    this.routes[voiceId] = fxTarget;
    if (this.engine && this.engine.updateVoiceRouting) {
      this.engine.updateVoiceRouting(voiceId, fxTarget);
    }
  }

  getRoute(voiceId) {
    return this.routes[voiceId] || 'direct';
  }
}

window.RoutingMatrix = RoutingMatrix;
