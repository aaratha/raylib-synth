#include "synth.h"
#include "rope.h"

typedef void (*SynthCallback)(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                              float *modPhase);

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
     .sequence = bassSequence,
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

// Initialize static variables
static float modPhases[MAX_INSTRUMENTS] = {0.0f};
static float filter_states[MAX_INSTRUMENTS] = {0.0f};

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

float calculate_alpha_cutoff(float cut_off) {
  float dt = 1.0f / SAMPLE_RATE;
  float RC = 1.0f / (2.0f * PI * cut_off);
  return dt / (RC + dt);
}

void lowpass_callback(float *sample, float *prev_y, float alpha) {
  float y = *prev_y + alpha * (*sample - *prev_y);
  *prev_y = y;
  *sample = y;
}

void rope_lowpass_callback(float *sample, float *prev_y, float min_frequency,
                           float max_frequency, float max_rope_length,
                           float rope_length) {
  float cut_off =
      lerp1D(min_frequency, max_frequency, rope_length / max_rope_length);
  float alpha = calculate_alpha_cutoff(cut_off);
  lowpass_callback(sample, prev_y, alpha);
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

// Common FM synthesis processing function
void process_fm_synthesis(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                          float *modPhase) {
  float modSignal = sinf(2.0f * PI * (*modPhase)) * fmSynth->modIndex;
  float fmFrequency = fmSynth->carrierFreq + (modSignal * fmSynth->carrierFreq);

  // Generate waveform
  float synthSample = shape_callback(fmSynth->carrierShape, fmSynth->phase);
  synthSample *= fmSynth->volume;

  // Update phase accumulators
  fmSynth->phase += fmFrequency / SAMPLE_RATE;
  if (fmSynth->phase >= 1.0f)
    fmSynth->phase -= 1.0f;

  *modPhase += fmSynth->modulatorFreq / SAMPLE_RATE;
  if (*modPhase >= 1.0f)
    *modPhase -= 1.0f;

  // Store in buffer and add to output
  fmSynth->buffer[frame % BUFFER_SIZE] = synthSample;
  *sample += synthSample;
}

void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase) {
  if (!sample || !fmSynth || !modPhase)
    return;

  fmSynth->carrierFreq = freq_from_rope_dir(&rope);
  float synthSample = 0.0f;
  process_fm_synthesis(&synthSample, frame, fmSynth, modPhase);

  float rope_length = Vector2Distance(rope.end, rope.start);
  float max_rope_length = 400;

  // Apply rope-based filtering
  rope_lowpass_callback(&synthSample, &filter_states[0], 100, 4000,
                        max_rope_length, rope_length);

  // Update modulator frequency based on rope length
  fmSynth->modulatorFreq = lerp1D(0, 6, rope_length / max_rope_length);

  *sample += synthSample; // Add the processed sample to the output
}

void random_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                           float *modPhase) {
  if (!sample || !fmSynth || !modPhase)
    return;

  if (globalControls.beat_triggered) {
    fmSynth->currentNote = GetRandomValue(0, 7);
  }

  fmSynth->carrierFreq =
      midi_to_freq(fmSynth->sequence[fmSynth->currentNote % 8]);

  float synthSample = 0.0f;
  process_fm_synthesis(&synthSample, frame, fmSynth, modPhase);

  // Apply envelope and filtering
  float beat_duration = 60.0f / BPM;
  float env_phase =
      fmodf(globalControls.beat_time, beat_duration) / beat_duration;
  envelope_callback(&synthSample, &env_phase, 0.1f, 0.9f, 0.0f, 0.0f);

  float rope_length = Vector2Distance(rope.end, rope.start);
  rope_lowpass_callback(&synthSample, &filter_states[1], 100, 4000, 400,
                        rope_length);

  *sample += synthSample; // Add the processed sample to the output
}

void const_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                          float *modPhase) {
  if (!sample || !fmSynth || !modPhase)
    return;

  if (globalControls.beat_triggered) {
    fmSynth->currentNote = GetRandomValue(0, 7);
  }

  fmSynth->carrierFreq =
      midi_to_freq(fmSynth->sequence[fmSynth->currentNote % 8]);
  float synthSample = 0.0f;
  process_fm_synthesis(&synthSample, frame, fmSynth, modPhase);
  *sample += synthSample; // Add the processed sample to the output
}

void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount) {
  if (!device || !output)
    return;

  float *out = (float *)output;
  static const SynthCallback callbacks[MAX_INSTRUMENTS] = {
      lead_synth_callback, random_synth_callback, const_synth_callback,
      const_synth_callback};

  // Process one frame at a time
  for (ma_uint32 i = 0; i < frameCount; i++) {
    float sample = 0.0f;
    bool was_triggered =
        globalControls.beat_triggered; // Store the current beat state

    // Process all instruments
    for (int j = 0; j < MAX_INSTRUMENTS; j++) {
      callbacks[j](&sample, i, &Instruments[j], &modPhases[j]);
    }

    // Only reset beat_triggered after processing all instruments for this frame
    if (was_triggered) {
      globalControls.beat_triggered = false;
    }

    sample /= MAX_INSTRUMENTS; // Prevent clipping

    // Write stereo output
    out[i * 2] = sample;
    out[i * 2 + 1] = sample;
  }
}
