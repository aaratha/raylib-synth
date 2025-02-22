#pragma once

#include "miniaudio.h"
#include "utils.h"

typedef struct {
  float frequency;
  float phase;
  float volume;
  int waveform; // 0: sine, 1: square, 2: triangle, 3: sawtooth
  float buffer[BUFFER_SIZE];
} Synthesizer;

typedef struct {
  float carrierFreq;
  int carrierShape;
  float modulatorFreq;
  float modIndex; // Depth of modulation
  float phase;
  float buffer[BUFFER_SIZE];
  int *sequence;
  int currentNote;
  float resonance;
  float volume;
} FMSynth;

typedef struct {
  float prev_x;  // Previous input
  float prev_y1; // Previous output 1
  float prev_y2; // Previous output 2
} ResonantFilter;

extern FMSynth Instruments[MAX_INSTRUMENTS];

float shape_callback(int shape, float t);

void lowpass_callback(float *sample, float *prev_y, float alpha);

void rope_lowpass_callback(float *sample, ResonantFilter *filter,
                           float rope_length, float resonance);

void envelope_callback(float *sample, float *phase, float attack, float decay,
                       float sustain, float release);

float calculate_alpha_cutoff(float cut_off);

void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase);
void random_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                           float *modPhase);
void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount);
