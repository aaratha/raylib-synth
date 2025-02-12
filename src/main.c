#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "raylib.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
// #define BUFFER_SIZE 1024
#define BUFFER_SIZE 400
#define MAX_INSTRUMENTS 4 // Number of simultaneous synths

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
  float modulatorFreq;
  float modIndex; // Depth of modulation
  float phase;
  float buffer[BUFFER_SIZE];
  float volume;
} FMSynth;

Synthesizer synth = {
    .frequency = 440.0f, .phase = 0.0f, .volume = 0.5f, .waveform = 0};

FMSynth Instruments[MAX_INSTRUMENTS] = {
    {.carrierFreq = 440.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 3.0f,
     .volume = 0.5f},
    {.carrierFreq = 660.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 2.0f,
     .volume = 0.5f},
    {.carrierFreq = 880.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 1.0f,
     .volume = 0.5f},
    {.carrierFreq = 220.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 4.0f,
     .volume = 0.5f},
};

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
  float baseRadius = 100.0f; // Base radius of the circle

  for (int s = 0; s < MAX_INSTRUMENTS; s++) {
    Color color = s == 0 ? RED : s == 1 ? GREEN : s == 2 ? BLUE : PURPLE;
    float prevAngle = 0;
    Vector2 prevPoint = center;

    for (int i = 0; i < BUFFER_SIZE; i++) {
      float angle = (i / (float)BUFFER_SIZE) * 2 * PI; // Angle in radians
      float radius =
          baseRadius + Instruments[s].buffer[i] * 50; // Modulate radius
      Vector2 point = {center.x + radius * cos(angle),
                       center.y + radius * sin(angle)};

      if (i > 0) {
        DrawLineV(prevPoint, point, color);
      }

      prevPoint = point;
      prevAngle = angle;
    }
  }
}

void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount) {
  float *out = (float *)output;

  for (ma_uint32 i = 0; i < frameCount; i++) {
    float sample = 0.0f;
    float t = synth.phase;

    switch (synth.waveform) {
    case SINE: // Sine wave
      sample = sinf(2.0f * PI * t);
      break;
    case SQUARE: // Square wave
      sample = t < 0.5f ? 1.0f : -1.0f;
      break;
    case TRIANGLE: // Triangle wave
      sample = (2.0f * fabsf(2.0f * t - 1.0f) - 1.0f);
      break;
    case SAWTOOTH: // Sawtooth wave
      sample = 2.0f * t - 1.0f;
      break;
    }

    sample *= synth.volume;
    synth.buffer[i % BUFFER_SIZE] = sample;

    // Output to both channels
    *out++ = sample;
    *out++ = sample;

    synth.phase += synth.frequency / SAMPLE_RATE;
    if (synth.phase >= 1.0f)
      synth.phase -= 1.0f;
  }
}

void fm_audio_callback(ma_device *device, void *output, const void *input,
                       ma_uint32 frameCount) {
  float *out = (float *)output;
  static float modPhases[MAX_INSTRUMENTS] = {0.0f};
  float sample;

  for (ma_uint32 i = 0; i < frameCount; i++) {
    sample = 0.0f; // Reset sample for mixing

    for (int j = 0; j < MAX_INSTRUMENTS; j++) {
      FMSynth *fmSynth = &Instruments[j];
      float *modPhase = &modPhases[j];

      float modSignal = sinf(2.0f * PI * (*modPhase)) * fmSynth->modIndex;
      float fmFrequency =
          fmSynth->carrierFreq + (modSignal * fmSynth->carrierFreq);
      float t = fmSynth->phase;
      float synthSample = sinf(2.0f * PI * t) * fmSynth->volume;
      sample += synthSample;

      fmSynth->buffer[i % BUFFER_SIZE] = synthSample;
      fmSynth->phase += fmFrequency / SAMPLE_RATE;
      if (fmSynth->phase >= 1.0f)
        fmSynth->phase -= 1.0f;
      *modPhase += fmSynth->modulatorFreq / SAMPLE_RATE;
      if (*modPhase >= 1.0f)
        *modPhase -= 1.0f;
    }

    // Prevent clipping by normalizing
    sample /= MAX_INSTRUMENTS;

    // Stereo output
    *out++ = sample;
    *out++ = sample;
  }
}

int main(void) {
  // Initialize audio device
  ma_device_config deviceConfig =
      ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_f32;
  deviceConfig.playback.channels = CHANNELS;
  deviceConfig.sampleRate = SAMPLE_RATE;
  deviceConfig.dataCallback = fm_audio_callback;

  ma_device device;
  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    return -1;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    ma_device_uninit(&device);
    return -1;
  }

  // Initialize window and graphics
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Trae Synth");
  SetTargetFPS(60);

  // Main game loop
  while (!WindowShouldClose()) {

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

    // Draw
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // draw_horizontal_waveforms();
    draw_circular_waveforms();

    DrawText(
        TextFormat("Modulator Freq: %.1f Hz", Instruments[0].modulatorFreq), 10,
        70, 20, BLACK);
    DrawText(TextFormat("Mod Index: %.1f", Instruments[0].modIndex), 10, 100,
             20, BLACK);

    EndDrawing();
  }

  // Cleanup
  ma_device_uninit(&device);
  CloseWindow();

  return 0;
}
