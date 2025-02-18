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
  float volume;
} FMSynth;

FMSynth Instruments[MAX_INSTRUMENTS];

float shape_callback(int shape, float t);
float lowpass(float x, float *prev_y, float alpha);
void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase);
void random_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                           float *modPhase);
void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount);
