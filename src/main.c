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
#define BUFFER_SIZE 1024

typedef struct {
  float frequency;
  float phase;
  float volume;
  int waveform; // 0: sine, 1: square, 2: triangle, 3: sawtooth
  float buffer[BUFFER_SIZE];
} Synthesizer;

Synthesizer synth = {
    .frequency = 440.0f, .phase = 0.0f, .volume = 0.5f, .waveform = 0};

void audio_callback(ma_device *device, void *output, const void *input,
                    ma_uint32 frameCount) {
  float *out = (float *)output;

  for (ma_uint32 i = 0; i < frameCount; i++) {
    float sample = 0.0f;
    float t = synth.phase;

    switch (synth.waveform) {
    case 0: // Sine wave
      sample = sinf(2.0f * PI * t);
      break;
    case 1: // Square wave
      sample = t < 0.5f ? 1.0f : -1.0f;
      break;
    case 2: // Triangle wave
      sample = (2.0f * fabsf(2.0f * t - 1.0f) - 1.0f);
      break;
    case 3: // Sawtooth wave
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

int main(void) {
  // Initialize audio device
  ma_device_config deviceConfig =
      ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_f32;
  deviceConfig.playback.channels = CHANNELS;
  deviceConfig.sampleRate = SAMPLE_RATE;
  deviceConfig.dataCallback = audio_callback;

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
    // Update synth parameters based on input
    if (IsKeyDown(KEY_UP))
      synth.frequency += 1.0f;
    if (IsKeyDown(KEY_DOWN))
      synth.frequency -= 1.0f;
    if (IsKeyPressed(KEY_SPACE))
      synth.waveform = (synth.waveform + 1) % 4;

    // Draw
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw waveform
    for (int i = 0; i < BUFFER_SIZE - 1; i++) {
      DrawLineEx((Vector2){i * (WINDOW_WIDTH / (float)BUFFER_SIZE),
                           WINDOW_HEIGHT / 2 + synth.buffer[i] * 100},
                 (Vector2){(i + 1) * (WINDOW_WIDTH / (float)BUFFER_SIZE),
                           WINDOW_HEIGHT / 2 + synth.buffer[i + 1] * 100},
                 2, BLUE);
    }

    // Draw UI text
    DrawText(TextFormat("Frequency: %.1f Hz", synth.frequency), 10, 10, 20,
             BLACK);
    DrawText(TextFormat("Waveform: %s", synth.waveform == 0   ? "Sine"
                                        : synth.waveform == 1 ? "Square"
                                        : synth.waveform == 2 ? "Triangle"
                                                              : "Sawtooth"),
             10, 40, 20, BLACK);

    EndDrawing();
  }

  // Cleanup
  ma_device_uninit(&device);
  CloseWindow();

  return 0;
}
