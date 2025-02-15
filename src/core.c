#include "core.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
// #define BUFFER_SIZE 1024
#define BUFFER_SIZE 400
#define MAX_INSTRUMENTS 4 // Number of simultaneous synths
#define ROPE_POINTS 10
#define ROPE_REST_LENGTH 100

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

typedef struct {
  vec2 start;
  vec2 end;
  vec2 end_prev;
  vec2 points[ROPE_POINTS];
  vec2 velocities[ROPE_POINTS]; // Added to store point velocities
  float damping;                // Added for energy loss
  float stiffness;              // Added for spring stiffness
  Color color;
} Rope;

/* Synthesizer synth = { */
/*     .frequency = 440.0f, .phase = 0.0f, .volume = 0.5f, .waveform = 0}; */

static ma_device device; // Global audio device
static Rope rope;        // Global rope instance

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

void init_rope(Rope *rope, vec2 start, vec2 end, Color color) {
  rope->start = start;
  rope->end_prev = end;
  rope->end = end;
  rope->color = color;
  rope->damping = 0.98f;  // Damping factor (energy loss)
  rope->stiffness = 0.5f; // Spring stiffness

  // Initialize points along the rope
  float dx = (end.x - start.x) / (ROPE_POINTS - 1);
  float dy = (end.y - start.y) / (ROPE_POINTS - 1);

  for (int i = 0; i < ROPE_POINTS; i++) {
    rope->points[i] = (vec2){start.x + dx * i, start.y + dy * i};
    rope->velocities[i] = (vec2){0, 0}; // Initialize velocities to zero
  }
}

void solve_rope_constraints(Rope *rope) {
  // Keep first point fixed
  rope->points[0] = rope->start;

  // Solve distance constraints
  for (int i = 0; i < ROPE_POINTS - 1; i++) {
    vec2 delta = Vector2Subtract(rope->points[i + 1], rope->points[i]);
    float dist = Vector2Length(delta);
    float target_dist = ROPE_REST_LENGTH / (float)(ROPE_POINTS - 1);

    if (dist > 0.0001f) {
      float difference = (dist - target_dist) / dist;
      vec2 correction = Vector2Scale(delta, 0.5f * difference);

      if (i > 0) {
        rope->points[i] = Vector2Add(rope->points[i], correction);
      }
      rope->points[i + 1] = Vector2Subtract(rope->points[i + 1], correction);
    }
  }
}

void update_rope(Rope *rope) {
  vec2 gravity = (vec2){0.0f, 0.0f}; // Increased gravity effect
  float dt = GetFrameTime();

  // Store current end position for velocity calculation
  rope->end = rope->points[ROPE_POINTS - 1];

  // Update velocities and positions for all points except the first
  for (int i = 1; i < ROPE_POINTS; i++) {
    // Apply gravity to velocity
    rope->velocities[i] =
        Vector2Add(rope->velocities[i], Vector2Scale(gravity, dt));

    // Update position using velocity
    rope->points[i] =
        Vector2Add(rope->points[i], Vector2Scale(rope->velocities[i], dt));

    // Apply damping
    rope->velocities[i] = Vector2Scale(rope->velocities[i], rope->damping);
  }

  // Constraint solving iterations
  const int ITERATIONS = 8;
  for (int iter = 0; iter < ITERATIONS; iter++) {
    solve_rope_constraints(rope);
  }

  // Mouse interaction
  vec2 mouse_pos = GetMousePosition();
  static bool was_dragging = false;
  bool is_dragging = false;

  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) &&
      Vector2Distance(mouse_pos, rope->points[ROPE_POINTS - 1]) < 20.0f) {
    // Dragging
    rope->points[ROPE_POINTS - 1] = mouse_pos;
    rope->end_prev = rope->end;
    rope->velocities[ROPE_POINTS - 1] = (vec2){0, 0};
    is_dragging = true;
    was_dragging = true;
  } else if (was_dragging && !IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    // Release with velocity
    vec2 release_velocity = Vector2Subtract(rope->end, rope->end_prev);
    rope->velocities[ROPE_POINTS - 1] = Vector2Scale(release_velocity, 60.0f);
    was_dragging = false;
  }

  // Boundary checking to keep rope on screen
  for (int i = 0; i < ROPE_POINTS; i++) {
    // Bottom boundary with bounce
    if (rope->points[i].y > WINDOW_HEIGHT - 10) {
      rope->points[i].y = WINDOW_HEIGHT - 10;
      rope->velocities[i].y =
          -rope->velocities[i].y * 0.5f; // Bounce with energy loss
    }

    // Side boundaries with bounce
    if (rope->points[i].x < 10) {
      rope->points[i].x = 10;
      rope->velocities[i].x = -rope->velocities[i].x * 0.5f;
    }
    if (rope->points[i].x > WINDOW_WIDTH - 10) {
      rope->points[i].x = WINDOW_WIDTH - 10;
      rope->velocities[i].x = -rope->velocities[i].x * 0.5f;
    }
  }
}

void draw_rope(Rope *rope) {
  DrawCircleV(rope->start, 5, rope->color);
  DrawCircleV(rope->end, 5, rope->color);
  for (int i = 0; i < ROPE_POINTS - 1; i++) {
    DrawLineEx(rope->points[i], rope->points[i + 1], 2, rope->color);
  }
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

/* void audio_callback(ma_device *device, void *output, const void *input, */
/*                     ma_uint32 frameCount) { */
/*   float *out = (float *)output; */

/*   for (ma_uint32 i = 0; i < frameCount; i++) { */
/*     float sample = 0.0f; */
/*     float t = synth.phase; */

/*     switch (synth.waveform) { */
/*     case SINE: // Sine wave */
/*       sample = sinf(2.0f * PI * t); */
/*       break; */
/*     case SQUARE: // Square wave */
/*       sample = t < 0.5f ? 1.0f : -1.0f; */
/*       break; */
/*     case TRIANGLE: // Triangle wave */
/*       sample = (2.0f * fabsf(2.0f * t - 1.0f) - 1.0f); */
/*       break; */
/*     case SAWTOOTH: // Sawtooth wave */
/*       sample = 2.0f * t - 1.0f; */
/*       break; */
/*     } */

/*     sample *= synth.volume; */
/*     synth.buffer[i % BUFFER_SIZE] = sample; */

/*     // Output to both channels */
/*     *out++ = sample; */
/*     *out++ = sample; */

/*     synth.phase += synth.frequency / SAMPLE_RATE; */
/*     if (synth.phase >= 1.0f) */
/*       synth.phase -= 1.0f; */
/*   } */
/* } */

void fm_audio_callback(ma_device *device, void *output, const void *input,
                       ma_uint32 frameCount) {
  float *out = (float *)output;
  static float modPhases[MAX_INSTRUMENTS] = {0.0f};
  static float carrierPhases[MAX_INSTRUMENTS] = {0.0f};
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

bool core_init_window(int width, int height, const char *title) {
  // Initialize audio device
  ma_device_config deviceConfig =
      ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_f32;
  deviceConfig.playback.channels = CHANNELS;
  deviceConfig.sampleRate = SAMPLE_RATE;
  deviceConfig.dataCallback = fm_audio_callback;

  vec2 center = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};

  init_rope(&rope, center, (vec2){center.x, center.x + 200}, RED);

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
