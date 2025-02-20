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
#define SCALE_SIZE 9

#define ROPE_POINTS 8
#define ROPE_REST_LENGTH 100
#define ROPE_THICKNESS 2
#define ROPE_ITERATIONS 5

#define SINE 0
#define SQUARE 1
#define TRIANGLE 2
#define SAWTOOTH 3

int constSequence[SEQ_SIZE];
int testSequence[SEQ_SIZE];
int pentatonicSequence[SEQ_SIZE];
int bassSequence[SEQ_SIZE];
int pentatonicScale[SCALE_SIZE];

typedef Vector2 vec2;

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
