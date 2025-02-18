#include "synth.h"
#include "rope.h"

FMSynth Instruments[MAX_INSTRUMENTS] = {
    {.carrierFreq = 440.0f,
     .carrierShape = SAWTOOTH,
     .modulatorFreq = 4.0f,
     .modIndex = 0.01f,
     .sequence = pentatonicSequence,
     .currentNote = 0,
     .volume = 0.0f},
    {.carrierFreq = 660.0f,
     .carrierShape = TRIANGLE,
     .modulatorFreq = 440.0f,
     .modIndex = 0.0f,
     .sequence = pentatonicSequence,
     .currentNote = 0,
     .volume = 0.0f},
    {.carrierFreq = 60.0f,
     .carrierShape = SAWTOOTH,
     .modulatorFreq = 440.0f,
     .modIndex = 0.0f,
     .sequence = constSequence,
     .currentNote = 0,
     .volume = 0.0f},
    {.carrierFreq = 220.0f,
     .carrierShape = SINE,
     .modulatorFreq = 440.0f,
     .modIndex = 0.0f,
     .sequence = constSequence,
     .currentNote = 0,
     .volume = 0.0f},
};

float shape_callback(int shape, float t) {
  switch (shape) {
  case SINE:
    return sinf(2.0f * PI * t);
  case SQUARE:
    return (t < 0.5f) ? 1.0f : -1.0f;
  case TRIANGLE:
    return 1.0f - 4.0f * fabsf(t - 0.5f);
  case SAWTOOTH:
    return 2.0f * t - 1.0f;
  default:
    return 0.0f;
  }
}

// One-pole low-pass filter: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
float lowpass(float x, float *prev_y, float alpha) {
  float y = *prev_y + alpha * (x - *prev_y);
  *prev_y = y;
  return y;
}

void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase) {
  /* if (globalControls.beat_triggered) { */
  /*   fmSynth->currentNote++; */
  /*   if (fmSynth->currentNote >= 8) */
  /*     fmSynth->currentNote = 0; */
  /* } */
  /* fmSynth->carrierFreq = fmSynth->sequence[fmSynth->currentNote]; */

  fmSynth->carrierFreq = freq_from_rope_dir(&rope);

  float modSignal = sinf(2.0f * PI * (*modPhase)) * fmSynth->modIndex;
  float fmFrequency = fmSynth->carrierFreq + (modSignal * fmSynth->carrierFreq);
  float t = fmSynth->phase;
  float synthSample;

  // Generate waveform based on carrier shape
  synthSample = shape_callback(fmSynth->carrierShape, t);

  synthSample *= fmSynth->volume;

  float rope_length = Vector2Distance(rope.end, rope.start);

  // --- Apply one-pole low pass filter with 1000 Hz cutoff ---
  float min_frequency = 100;
  float max_frequency = 4000;
  float min_rope_length = 10;
  float max_rope_length = 400;

  float cut_off =
      lerp1D(min_frequency, max_frequency, rope_length / max_rope_length);
  float dt = 1.0f / SAMPLE_RATE;
  float RC = 1.0f / (2.0f * PI * cut_off); // Time constant for a 1000 Hz cutoff
  float alpha = dt / (RC + dt);            // Compute filter coefficient
  static float lead_filter_prev = 0.0f;    // Persistent filter state
  float filteredSample = lowpass(synthSample, &lead_filter_prev, alpha);

  float min_mod_freq = 0;
  float max_mod_freq = 6;
  fmSynth->modulatorFreq =
      lerp1D(min_mod_freq, max_mod_freq, rope_length / max_rope_length);

  *sample += filteredSample;

  fmSynth->buffer[frame % BUFFER_SIZE] = synthSample;
  fmSynth->phase += fmFrequency / SAMPLE_RATE;
  if (fmSynth->phase >= 1.0f)
    fmSynth->phase -= 1.0f;
  *modPhase += fmSynth->modulatorFreq / SAMPLE_RATE;
  if (*modPhase >= 1.0f)
    *modPhase -= 1.0f;
}

void random_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                           float *modPhase) {
  if (globalControls.beat_triggered) {
    fmSynth->currentNote = GetRandomValue(0, 7);
  }
  fmSynth->carrierFreq = midi_to_freq(fmSynth->sequence[fmSynth->currentNote]);

  float modSignal = sinf(2.0f * PI * (*modPhase)) * fmSynth->modIndex;
  float fmFrequency = fmSynth->carrierFreq + (modSignal * fmSynth->carrierFreq);
  float t = fmSynth->phase;
  float synthSample;

  // Generate waveform based on carrier shape
  synthSample = shape_callback(fmSynth->carrierShape, t);

  synthSample =
      lowpass(synthSample, &fmSynth->buffer[frame % BUFFER_SIZE], 0.9f);

  synthSample *= fmSynth->volume;
  *sample += synthSample;

  fmSynth->buffer[frame % BUFFER_SIZE] = synthSample;
  fmSynth->phase += fmFrequency / SAMPLE_RATE;
  if (fmSynth->phase >= 1.0f)
    fmSynth->phase -= 1.0f;
  *modPhase += fmSynth->modulatorFreq / SAMPLE_RATE;
  if (*modPhase >= 1.0f)
    *modPhase -= 1.0f;
}

void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount) {
  float *out = (float *)output;
  static float modPhases[MAX_INSTRUMENTS] = {0.0f};
  static float carrierPhases[MAX_INSTRUMENTS] = {0.0f};
  float sample;

  if (globalControls.time >= 60.0f / globalControls.bpm) {
    globalControls.time = 0;
    globalControls.beat_triggered = true;
  }

  for (ma_uint32 i = 0; i < frameCount; i++) {
    sample = 0.0f; // Reset sample for mixing

    for (int j = 0; j < MAX_INSTRUMENTS; j++) {
      FMSynth *fmSynth = &Instruments[j];
      float *modPhase = &modPhases[j];

      switch (j) {
      case 0:
        lead_synth_callback(&sample, i, fmSynth, modPhase);
        break;
      case 1:
        random_synth_callback(&sample, i, fmSynth, modPhase);
        break;
      case 2:
        random_synth_callback(&sample, i, fmSynth, modPhase);
        break;
      case 3:
        random_synth_callback(&sample, i, fmSynth, modPhase);
        break;
      }
    }
    globalControls.beat_triggered = false;

    // Prevent clipping by normalizing
    sample /= MAX_INSTRUMENTS;

    // Stereo output
    *out++ = sample;
    *out++ = sample;
  }
}
