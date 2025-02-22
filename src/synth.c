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
     .resonance = 2.0f,
     .volume = 0.0f},
    {.carrierFreq = 660.0f,
     .carrierShape = SQUARE,
     .modulatorFreq = 440.0f,
     .modIndex = 0.0f,
     .sequence = bassSequence,
     .currentNote = 0,
     .resonance = 2.0f,
     .volume = 0.0f},
    {.carrierFreq = 60.0f,
     .carrierShape = SAWTOOTH,
     .modulatorFreq = 440.0f,
     .modIndex = 0.0f,
     .sequence = constSequence,
     .currentNote = 0,
     .resonance = 2.0f,
     .volume = 0.0f},
    {.carrierFreq = 220.0f,
     .carrierShape = SINE,
     .modulatorFreq = 440.0f,
     .modIndex = 0.0f,
     .sequence = constSequence,
     .currentNote = 0,
     .resonance = 2.0f,
     .volume = 0.0f},
};

// Initialize static variables
static float modPhases[MAX_INSTRUMENTS] = {0.0f};
static ResonantFilter filter_states[MAX_INSTRUMENTS] = {0};

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

void resonant_lowpass_callback(float *sample, ResonantFilter *filter,
                               float cutoff, float resonance) {
  // Constrain parameters to stable ranges
  cutoff = fminf(fmaxf(cutoff, 20.0f), SAMPLE_RATE / 2.0f);
  resonance = fmaxf(resonance, 0.0f); // Resonance >= 0

  // Calculate filter coefficients
  float w0 = 2.0f * PI * cutoff / SAMPLE_RATE;
  float alpha = sinf(w0) / (2.0f * (1.0f + resonance));
  float cosw0 = cosf(w0);

  float a0 = 1.0f + alpha;
  float a1 = -2.0f * cosw0;
  float a2 = 1.0f - alpha;
  float b0 = (1.0f - cosw0) / 2.0f;
  float b1 = 1.0f - cosw0;
  float b2 = (1.0f - cosw0) / 2.0f;

  // Normalize coefficients
  a1 /= a0;
  a2 /= a0;
  b0 /= a0;
  b1 /= a0;
  b2 /= a0;

  // Process sample
  float x = *sample;
  float y = b0 * x + b1 * filter->prev_x + b2 * filter->prev_x -
            a1 * filter->prev_y1 - a2 * filter->prev_y2;

  // Update filter state
  filter->prev_x = x;
  filter->prev_y2 = filter->prev_y1;
  filter->prev_y1 = y;

  *sample = y;
}

void rope_lowpass_callback(float *sample, ResonantFilter *filter,
                           float rope_length, float resonance) {
  float cut_off = lerp1D(MIN_CUTOFF_FREQUENCY, MAX_CUTOFF_FREQUENCY,
                         rope_length / MAX_ROPE_LENGTH);
  float alpha = calculate_alpha_cutoff(cut_off);
  resonant_lowpass_callback(sample, filter, cut_off, resonance);
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
  rope_lowpass_callback(&synthSample, &filter_states[0], rope_length,
                        fmSynth->resonance);

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
  rope_lowpass_callback(&synthSample, &filter_states[1], rope_length,
                        fmSynth->resonance);

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
