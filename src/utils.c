#include "utils.h"

int constSequence[SEQ_SIZE] = {
    A3, A3, A3, A3, A3, A3, A3, A3,
};

int testSequence[SEQ_SIZE] = {
    A3, Af3, B3, C4, D4, E4, F4, G4,
};

int pentatonicSequence[SEQ_SIZE] = {C3, D3, F3, G3, Af3, C4, D4, F4};

// pentatonic scale in order
// C–D–F–G–B♭–C
int pentatonicScale[SCALE_SIZE] = {
    C3, D3, F3, G3, Af3, C4, D4, F4, G4,
};

float lerp1D(float a, float b, float t) { return a + t * (b - a); }
vec2 lerp2D(vec2 a, vec2 b, float t) {
  return (vec2){lerp1D(a.x, b.x, t), lerp1D(a.y, b.y, t)};
}

float midi_to_freq(int midi) {
  return powf(2.0f, (midi - 69) / 12.0f) * 440.0f;
}

void init_globalControls(GlobalControls *globalControls) {
  globalControls->bpm = 120;
  globalControls->time = 0;
  globalControls->beat_triggered = false;
}
