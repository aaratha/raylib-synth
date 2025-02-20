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
     .carrierShape = SQUARE,
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

void envelope_callback(float *sample, float *phase, float attack, float decay,
                       float sustain, float release) {
  if (*phase < attack) {
    *sample *= *phase / attack;
  } else if (*phase < attack + decay) {
    *sample *= lerp1D(1.0f, sustain, (*phase - attack) / decay);
  } else if (*phase < 1.0f - release) {
    *sample *= sustain;
  } else {
    *sample *= lerp1D(sustain, 0.0f, (*phase - (1.0f - release)) / release);
  }
}

float calculate_alpha_cutoff(float cut_off) {
  float dt = 1.0f / SAMPLE_RATE;
  float RC = 1.0f / (2.0f * PI * cut_off); // Time constant for a 1000 Hz cutoff
  float alpha = dt / (RC + dt);            // Compute filter coefficient
  return alpha;
}

// Initialize static variables
static float modPhases[MAX_INSTRUMENTS] = {0.0f};
static float lead_filter_prev = 0.0f;

void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase) {
  if (!sample || !fmSynth || !modPhase) return;

  fmSynth->carrierFreq = freq_from_rope_dir(&rope);

  float modSignal = sinf(2.0f * PI * (*modPhase)) * fmSynth->modIndex;
  float fmFrequency = fmSynth->carrierFreq + (modSignal * fmSynth->carrierFreq);
  float t = fmSynth->phase;
  float synthSample;

  // Generate waveform based on carrier shape
  synthSample = shape_callback(fmSynth->carrierShape, t);

  synthSample *= fmSynth->volume;

  float rope_length = Vector2Distance(rope.end, rope.start);

  float min_frequency = 100;
  float max_frequency = 4000;
  float min_rope_length = 10;
  float max_rope_length = 400;

  float cut_off =
      lerp1D(min_frequency, max_frequency, rope_length / max_rope_length);

  float alpha = calculate_alpha_cutoff(cut_off);

  float filteredSample = lowpass(synthSample, &lead_filter_prev, alpha);

  float min_mod_freq = 0;
  float max_mod_freq = 6;
  fmSynth->modulatorFreq =
      lerp1D(min_mod_freq, max_mod_freq, rope_length / max_rope_length);

  *sample += filteredSample;

  // Ensure buffer access is within bounds
  ma_uint32 buffer_index = frame % BUFFER_SIZE;
  fmSynth->buffer[buffer_index] = synthSample;
  
  fmSynth->phase += fmFrequency / SAMPLE_RATE;
  if (fmSynth->phase >= 1.0f)
    fmSynth->phase -= 1.0f;
  *modPhase += fmSynth->modulatorFreq / SAMPLE_RATE;
  if (*modPhase >= 1.0f)
    *modPhase -= 1.0f;
}

void random_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                           float *modPhase) {
  if (!sample || !fmSynth || !modPhase) return;

  if (globalControls.beat_triggered) {
    fmSynth->currentNote = GetRandomValue(0, 7);
  }
  
  // Ensure currentNote is within bounds of sequence array
  if (fmSynth->currentNote >= 8) fmSynth->currentNote = 0;
  fmSynth->carrierFreq = midi_to_freq(fmSynth->sequence[fmSynth->currentNote]);

  float modSignal = sinf(2.0f * PI * (*modPhase)) * fmSynth->modIndex;
  float fmFrequency = fmSynth->carrierFreq + (modSignal * fmSynth->carrierFreq);
  float t = fmSynth->phase;
  float synthSample;

  // Generate waveform based on carrier shape
  synthSample = shape_callback(fmSynth->carrierShape, t);

  // Ensure buffer access is within bounds
  ma_uint32 buffer_index = frame % BUFFER_SIZE;
  synthSample = lowpass(synthSample, &fmSynth->buffer[buffer_index], 0.9f);

  float env_phase = globalControls.time / (60.0f / globalControls.bpm);

  // attack, decay, sustain, release
  envelope_callback(&synthSample, &env_phase, 0.0f, 1.0f, 0.0f, 0.0f);

  synthSample *= fmSynth->volume;
  *sample += synthSample;

  fmSynth->buffer[buffer_index] = synthSample;
  
  fmSynth->phase += fmFrequency / SAMPLE_RATE;
  if (fmSynth->phase >= 1.0f)
    fmSynth->phase -= 1.0f;
  *modPhase += fmSynth->modulatorFreq / SAMPLE_RATE;
  if (*modPhase >= 1.0f)
    *modPhase -= 1.0f;
}

void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount) {
  if (!device || !output) return;

  float *out = (float *)output;
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
      case 2:
      case 3:
        random_synth_callback(&sample, i, fmSynth, modPhase);
        break;
      }
    }
    globalControls.beat_triggered = false;

    // Prevent clipping by normalizing
    sample /= MAX_INSTRUMENTS;

    // Ensure we don't write beyond the output buffer
    if (i * 2 + 1 < frameCount * 2) {
      // Stereo output
      out[i * 2] = sample;
      out[i * 2 + 1] = sample;
    }
  }
}
