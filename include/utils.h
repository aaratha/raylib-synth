#pragma once

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BPM 60
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define BUFFER_SIZE 400   // 1024
#define MAX_INSTRUMENTS 4 // Number of simultaneous synths
#define SEQ_SIZE 8
#define SCALE_SIZE 10

#define ROPE_POINTS 15
#define ROPE_REST_LENGTH 100
#define ROPE_THICKNESS 2
#define ROPE_ITERATIONS 10
#define MAX_ROPE_LENGTH 400

#define MIN_WAVEFORM_RADIUS 100
#define MAX_WAVEFORM_RADIUS 250
#define WAVEFORM_AMPLITUDE_MULTIPLIER 20
#define MIN_WAVEFORM_SEPARATION 30
#define MAX_WAVEFORM_SEPARATION 50

#define MIN_CUTOFF_FREQUENCY 100
#define MAX_CUTOFF_FREQUENCY 10000

#define MIN_GRIDLINE_RADIUS 60
#define MAX_GRIDLINE_RADIUS 110
#define NOTE_DISPLAY_RADIUS 80

#define GRAPHICS_LERP_SPEED 4

extern int constSequence[SEQ_SIZE];
extern int testSequence[SEQ_SIZE];
extern int pentatonicSequence[SEQ_SIZE];
extern int bassSequence[SEQ_SIZE];
extern int arpeggioSequence[SEQ_SIZE];
extern int pentatonicScale[SCALE_SIZE];

typedef Vector2 vec2;

enum Waveforms { SINE = 0, SQUARE = 1, TRIANGLE = 2, SAWTOOTH = 3 };

enum Notes {
  C1 = 24,
  Cs1 = 25,
  D1 = 26,
  Ds1 = 27,
  E1 = 28,
  F1 = 29,
  Fs1 = 30,
  G1 = 31,
  Gs1 = 32,
  A1 = 33,
  Af1 = 34,
  B1 = 35,
  C2 = 36,
  Cs2 = 37,
  D2 = 38,
  Ds2 = 39,
  E2 = 40,
  F2 = 41,
  Fs2 = 42,
  G2 = 43,
  Gs2 = 44,
  A2 = 45,
  Af2 = 46,
  B2 = 47,
  C3 = 48,
  Cs3 = 49,
  D3 = 50,
  Ds3 = 51,
  E3 = 52,
  F3 = 53,
  Fs3 = 54,
  G3 = 55,
  Gs3 = 56,
  A3 = 57,
  Af3 = 58,
  B3 = 59,
  C4 = 60,
  Cs4 = 61,
  D4 = 62,
  Ds4 = 63,
  E4 = 64,
  F4 = 65,
  Fs4 = 66,
  G4 = 67,
  Gs4 = 68,
  A4 = 69,
  Af4 = 70,
  B4 = 71,
  C5 = 72,
  Cs5 = 73,
  D5 = 74,
  Ds5 = 75,
  E5 = 76,
  F5 = 77,
  Fs5 = 78,
  G5 = 79,
  Gs5 = 80,
  A5 = 81,
  Af5 = 82,
  B5 = 83
};

typedef struct {
  float physics_time;
  float beat_time;
  bool beat_triggered;
} GlobalControls;

float lerp1D(float a, float b, float t);
vec2 lerp2D(vec2 a, vec2 b, float t);

float midi_to_freq(int midi);

void init_globalControls(GlobalControls *globalControls);

extern GlobalControls globalControls;
