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

typedef struct {
  float attack;
  float decay;
  float sustain;
  float release;
  float phase;
} EnvControls;

typedef struct {
  float delayTime;
  float feedback;
  float wet;
} DelayControls;

extern FMSynth Instruments[MAX_INSTRUMENTS];

float generate_shape(int shape, float t);

float calculate_alpha_cutoff(float cut_off);

void lowpass_callback(float *sample, float *prev_y, float alpha);

void rope_lowpass_callback(float *sample, ResonantFilter *filter,
                           float rope_length, float resonance);

void envelope_callback(float *sample, EnvControls *envControls);

void delay_callback(float *sample, float *buffer, float *delay_time,
                    float *feedback, float *wet);

void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase);

void rhythm_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                           float *modPhase);

void arpeggio_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                             float *modPhase);

void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount);
