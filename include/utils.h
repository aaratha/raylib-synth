#pragma once

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
// #define BUFFER_SIZE 1024
#define BUFFER_SIZE 400
#define MAX_INSTRUMENTS 4 // Number of simultaneous synths
#define ROPE_POINTS 10
#define ROPE_REST_LENGTH 100
#define SEQ_SIZE 8
#define SCALE_SIZE 9

#define SINE 0
#define SQUARE 1
#define TRIANGLE 2
#define SAWTOOTH 3

int constSequence[SEQ_SIZE];
int testSequence[SEQ_SIZE];
int pentatonicSequence[SEQ_SIZE];
int pentatonicScale[SCALE_SIZE];

typedef Vector2 vec2;

enum Notes {
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
  int bpm;
  float time;
  bool beat_triggered;
} GlobalControls;

float lerp1D(float a, float b, float t);
vec2 lerp2D(vec2 a, vec2 b, float t);

float midi_to_freq(int midi);

void init_globalControls(GlobalControls *globalControls);

extern GlobalControls globalControls;
