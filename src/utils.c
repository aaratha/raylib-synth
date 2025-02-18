#include "utils.h"

float lerp1D(float a, float b, float t) { return a + t * (b - a); }
vec2 lerp2D(vec2 a, vec2 b, float t) {
  return (vec2){lerp1D(a.x, b.x, t), lerp1D(a.y, b.y, t)};
}

float midi_to_freq(int midi) {
  return powf(2.0f, (midi - 69) / 12.0f) * 440.0f;
}
