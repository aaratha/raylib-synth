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

typedef Vector2 vec2;

float lerp1D(float a, float b, float t);
vec2 lerp2D(vec2 a, vec2 b, float t);

float midi_to_freq(int midi);
