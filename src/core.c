#include "core.h"
#include "graphics.h"
#include "rope.h"
#include "synth.h"
#include "utils.h"
#include <raylib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static ma_device device; // Global audio device
Rope rope;
GlobalControls globalControls;

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
  // SetTargetFPS(60);
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
    Instruments[0].volume =
        Instruments[0].volume == 0.0f ? DEFAULT_LEAD_VOLUME : 0.0f;
  if (IsKeyPressed(KEY_TWO))
    Instruments[1].volume =
        Instruments[1].volume == 0.0f ? DEFAULT_BASS_VOLUME : 0.0f;
  if (IsKeyPressed(KEY_THREE))
    Instruments[2].volume =
        Instruments[2].volume == 0.0f ? DEFAULT_ARPEGGIO_VOLUME : 0.0f;
  if (IsKeyPressed(KEY_FOUR))
    Instruments[3].volume = Instruments[3].volume == 0.0f ? 0.5f : 0.0f;

  globalControls.physics_time += GetFrameTime();
  globalControls.beat_time += GetFrameTime();
  globalControls.sub_beat_time += GetFrameTime();

  if (globalControls.beat_time >= 60.0f / globalControls.bpm) {
    globalControls.beat_time = 0;
    globalControls.beat_triggered = true;
  }

  if (globalControls.sub_beat_time >= 60.0f / globalControls.bpm / SUB_BEATS) {
    globalControls.sub_beat_time = 0;
    globalControls.sub_beat_triggered = true;
  }

  if (globalControls.physics_time >= 1.0f / 60.0f) {
    globalControls.physics_time = 0;
    update_rope(&rope);
  }

  rope_bpm_controller(&rope, &globalControls);

  // Draw
  BeginDrawing();
  ClearBackground(RAYWHITE);

  draw_note_grid(rope.start, rope.end);
  // draw_horizontal_waveforms();
  draw_circular_waveforms();

  draw_rope(&rope);

  DrawText(TextFormat("Modulator Freq: %.1f Hz", Instruments[0].modulatorFreq),
           10, 70, 20, BLACK);
  DrawText("Press: 1, 2, 3, or 4", 10, 100, 20, BLACK);
  DrawFPS(10, 10);

  EndDrawing();
}
