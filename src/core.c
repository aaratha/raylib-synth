#include "core.h"
#include "rope.h"
#include "utils.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

const int SINE = 0;
const int SQUARE = 1;
const int TRIANGLE = 2;
const int SAWTOOTH = 3;

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

typedef struct {
  int bpm;
  float time;
  bool beat_triggered;
} GlobalControls;

// fix notes struct

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

static ma_device device; // Global audio device
static Rope rope;        // Global rope instance
static GlobalControls globalControls;

int constSequence[SEQ_SIZE] = {
    A3, A3, A3, A3, A3, A3, A3, A3,
};

int testSequence[SEQ_SIZE] = {
    A3, Af3, B3, C4, D4, E4, F4, G4,
};

int pentatonicSequence[SEQ_SIZE] = {C3, D3, F3, G3, Af3, C4, D4, F4};

// pentatonic scale in order
// C–D–F–G–B♭–C
int pentatonicScale[SCALE_SIZE] = {
    C3, D3, F3, G3, Af3, C4, D4, F4, G4,
};

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

void init_globalControls(GlobalControls *globalControls) {
  globalControls->bpm = 120;
  globalControls->time = 0;
  globalControls->beat_triggered = false;
}

void draw_horizontal_waveforms() {
  for (int s = 0; s < MAX_INSTRUMENTS; s++) {
    float y_pos = WINDOW_HEIGHT / 2 + s * 100 - 150;
    Color color = s == 0 ? RED : s == 1 ? GREEN : s == 2 ? BLUE : PURPLE;
    for (int i = 0; i < BUFFER_SIZE - 1; i++) {
      DrawLineEx((Vector2){i * (WINDOW_WIDTH / (float)BUFFER_SIZE),
                           y_pos + Instruments[s].buffer[i] * 100},
                 (Vector2){(i + 1) * (WINDOW_WIDTH / (float)BUFFER_SIZE),
                           y_pos + Instruments[s].buffer[i + 1] * 100},
                 2, color);
    }
  }
}

void draw_circular_waveforms() {
  // Circle parameters
  Vector2 center = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
  float baseRadius = 200.0f; // Base radius of the circle
  float thickness = 2.0f;    // Thickness of the waveform

  float minBrightness = 100;
  float maxBrightness = 200;

  for (int s = 0; s < MAX_INSTRUMENTS; s++) {
    float brightness =
        (s / (float)MAX_INSTRUMENTS) * (maxBrightness - minBrightness) +
        minBrightness;
    Color color = (Color){brightness, brightness, brightness, 255};
    float specificRadius = baseRadius + s * 30;
    Vector2 prevPoint = {center.x + specificRadius,
                         center.y}; // Start with the first calculated point
    Vector2 firstPoint = prevPoint;

    for (int i = 0; i < BUFFER_SIZE; i++) {
      float angle = (i / (float)BUFFER_SIZE) * 2 * PI; // Angle in radians
      float radius =
          specificRadius + Instruments[s].buffer[i] * 30; // Modulate radius
      Vector2 point = {center.x + radius * cos(angle),
                       center.y + radius * sin(angle)};

      if (i == 0) {
        firstPoint =
            point; // Store the first point in case we need to close the loop
      }

      if (i == BUFFER_SIZE - 1) {
        DrawLineEx(prevPoint, firstPoint, thickness, color); // Close the loop
      } else {
        DrawLineEx(prevPoint, point, thickness, color);
      }

      prevPoint = point;
    }
  }
}

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

float freq_from_rope_dir() {
  vec2 direction = Vector2Subtract(rope.end, rope.start);
  float angle = atan2f(direction.y, direction.x);
  float angle_deg = angle * 180 / PI;

  // choose frequency based on angle from pentatonicScale
  float step_size = 360.0f / SCALE_SIZE;
  int index = (int)((angle_deg + 180.0f) / step_size) % SCALE_SIZE;
  float frequency = midi_to_freq(pentatonicScale[index]);

  return frequency;
}

void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase) {
  /* if (globalControls.beat_triggered) { */
  /*   fmSynth->currentNote++; */
  /*   if (fmSynth->currentNote >= 8) */
  /*     fmSynth->currentNote = 0; */
  /* } */
  /* fmSynth->carrierFreq = fmSynth->sequence[fmSynth->currentNote]; */

  fmSynth->carrierFreq = freq_from_rope_dir();

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

bool core_init_window(const char *title) {
  // Initialize audio device
  ma_device_config deviceConfig =
      ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_f32;
  deviceConfig.playback.channels = CHANNELS;
  deviceConfig.sampleRate = SAMPLE_RATE;
  deviceConfig.dataCallback = audio_callback;

  init_globalControls(&globalControls);

  vec2 center = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};

  init_rope(&rope, center, (vec2){center.x, center.x + 200}, GRAY);

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    return -1;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    ma_device_uninit(&device);
    return -1;
  }

  // Initialize window and graphics
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title);
  SetTargetFPS(60);
  return true;
}

void core_close_window() {
  ma_device_uninit(&device);
  CloseWindow();
}

bool core_window_should_close() { return WindowShouldClose(); }

void core_execute_loop() {
  if (IsKeyDown(KEY_W))
    Instruments[0].modulatorFreq += 1.0f;
  if (IsKeyDown(KEY_S))
    Instruments[0].modulatorFreq -= 1.0f;
  if (IsKeyDown(KEY_A))
    Instruments[0].modIndex += 0.1f;
  if (IsKeyDown(KEY_D))
    Instruments[0].modIndex -= 0.1f;

  if (IsKeyPressed(KEY_ONE))
    Instruments[0].volume = Instruments[0].volume == 0.0f ? 0.5f : 0.0f;
  if (IsKeyPressed(KEY_TWO))
    Instruments[1].volume = Instruments[1].volume == 0.0f ? 0.5f : 0.0f;
  if (IsKeyPressed(KEY_THREE))
    Instruments[2].volume = Instruments[2].volume == 0.0f ? 0.5f : 0.0f;
  if (IsKeyPressed(KEY_FOUR))
    Instruments[3].volume = Instruments[3].volume == 0.0f ? 0.5f : 0.0f;

  globalControls.time += GetFrameTime();

  // Update
  update_rope(&rope);

  // Draw
  BeginDrawing();
  ClearBackground(RAYWHITE);

  // draw_horizontal_waveforms();
  draw_circular_waveforms();

  draw_rope(&rope);

  DrawText(TextFormat("Modulator Freq: %.1f Hz", Instruments[0].modulatorFreq),
           10, 70, 20, BLACK);
  DrawText(TextFormat("Mod Index: %.1f", Instruments[0].modIndex), 10, 100, 20,
           BLACK);

  EndDrawing();
}
