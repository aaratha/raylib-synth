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
  int bpm;
  float sequence[8];
  int currentNote;
  float time;
} AudioControls;

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

typedef struct {
  float A3;
  float Af3;
  float A4;
  float Af4;
  float B3;
  float Bf3;
  float B4;
  float Bf4;
  float C3;
  float Cf3;
  float C4;
  float Cf4;
  float D3;
  float Df3;
  float D4;
  float Df4;
  float E3;
  float Ef3;
  float E4;
  float Ef4;
  float F3;
  float Ff3;
  float F4;
  float Ff4;
  float G3;
  float Gf3;
  float G4;
  float Gf4;
} NoteFrequencies;

const NoteFrequencies notes = {
    .A3 = 220.0f,
    .Af3 = 207.65f,
    .A4 = 440.0f,
    .Af4 = 415.30f,
    .B3 = 246.94f,
    .Bf3 = 233.08f,
    .B4 = 493.88f,
    .Bf4 = 466.16f,
    .C3 = 130.81f,
    .Cf3 = 123.47f,
    .C4 = 261.63f,
    .Cf4 = 277.18f,
    .D3 = 146.83f,
    .Df3 = 138.59f,
    .D4 = 293.66f,
    .Df4 = 277.18f,
    .E3 = 164.81f,
    .Ef3 = 155.56f,
    .E4 = 329.63f,
    .Ef4 = 311.13f,
    .F3 = 174.61f,
    .Ff3 = 164.81f,
    .F4 = 349.23f,
    .Ff4 = 329.63f,
    .G3 = 196.00f,
    .Gf3 = 185.00f,
    .G4 = 392.00f,
    .Gf4 = 369.99f,
};

static ma_device device; // Global audio device
static Rope rope;        // Global rope instance
static AudioControls audioControls;

FMSynth Instruments[MAX_INSTRUMENTS] = {
    {.carrierFreq = 440.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 3.0f,
     .volume = 0.0f},
    {.carrierFreq = 660.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 2.0f,
     .volume = 0.0f},
    {.carrierFreq = 880.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 1.0f,
     .volume = 0.0f},
    {.carrierFreq = 220.0f,
     .modulatorFreq = 440.0f,
     .modIndex = 4.0f,
     .volume = 0.0f},
};

float lerp1D(float a, float b, float t) { return a + t * (b - a); }
vec2 lerp2D(vec2 a, vec2 b, float t) {
  return (vec2){lerp1D(a.x, b.x, t), lerp1D(a.y, b.y, t)};
}

void init_audioControls(AudioControls *audioControls) {
  audioControls->bpm = 120;
  audioControls->time = 0;
  audioControls->currentNote = 0;
  audioControls->sequence[0] = notes.A3;
  audioControls->sequence[1] = notes.Af3;
  audioControls->sequence[2] = notes.B3;
  audioControls->sequence[3] = notes.C4;
  audioControls->sequence[4] = notes.D4;
  audioControls->sequence[5] = notes.E4;
  audioControls->sequence[6] = notes.F4;
  audioControls->sequence[7] = notes.G4;
}

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
  vec2 gravity = (vec2){0.0f, 800.0f}; // Apply downward gravity
  float dt = GetFrameTime();
  if (dt <= 0)
    return;

  // Store current end position for velocity calculation (before any updates)
  static vec2 prev_end_pos;
  vec2 current_end_before_update = rope->points[ROPE_POINTS - 1];

  // Store old positions for velocity correction after constraints
  vec2 old_points[ROPE_POINTS];
  for (int i = 0; i < ROPE_POINTS; i++) {
    old_points[i] = rope->points[i];
  }

  // Apply forces and integrate positions (semi-implicit Euler)
  for (int i = 1; i < ROPE_POINTS; i++) {
    // Apply gravity to velocity
    rope->velocities[i] =
        Vector2Add(rope->velocities[i], Vector2Scale(gravity, dt));

    // Update position using velocity
    rope->points[i] =
        Vector2Add(rope->points[i], Vector2Scale(rope->velocities[i], dt));
  }

  // Solve constraints multiple times for stability
  const int ITERATIONS = 8;
  for (int iter = 0; iter < ITERATIONS; iter++) {
    solve_rope_constraints(rope);
  }

  // Update velocities based on constrained positions and apply damping
  for (int i = 0; i < ROPE_POINTS; i++) {
    vec2 displacement = Vector2Subtract(rope->points[i], old_points[i]);
    rope->velocities[i] = Vector2Scale(displacement, 1.0f / dt);
    rope->velocities[i] = Vector2Scale(rope->velocities[i], rope->damping);
  }

  // Mouse interaction
  vec2 mouse_pos = GetMousePosition();
  static bool was_dragging = false;
  static vec2 drag_start_pos;

  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    float grab_radius = 100.0f;
    if (CheckCollisionPointCircle(mouse_pos, rope->points[ROPE_POINTS - 1],
                                  grab_radius) ||
        was_dragging) {
      // Start dragging
      rope->points[ROPE_POINTS - 1] = mouse_pos;
      //    lerp2D(rope->points[ROPE_POINTS - 1], mouse_pos, 0.5f);
      rope->velocities[ROPE_POINTS - 1] = Vector2Zero();
      was_dragging = true;
      drag_start_pos = mouse_pos;
    }
  } else if (was_dragging) {
    // Release with velocity based on drag motion
    vec2 drag_vector = Vector2Subtract(mouse_pos, drag_start_pos);
    rope->velocities[ROPE_POINTS - 1] = Vector2Scale(drag_vector, 1.0f / dt);
    was_dragging = false;
  }

  // Maintain fixed starting point (optional)
  rope->end = rope->points[ROPE_POINTS - 1];
  rope->points[0] = rope->start;
  rope->velocities[0] = Vector2Zero();
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

void lead_synth_callback(float *sample, ma_uint32 frame, FMSynth *fmSynth,
                         float *modPhase) {
  if (audioControls.time >= 60.0f / audioControls.bpm) {
    audioControls.time = 0;
    audioControls.currentNote++;
    if (audioControls.currentNote >= 8)
      audioControls.currentNote = 0;
  }
  fmSynth->carrierFreq = audioControls.sequence[audioControls.currentNote];

  float modSignal = sinf(2.0f * PI * (*modPhase)) * fmSynth->modIndex;
  float fmFrequency = fmSynth->carrierFreq + (modSignal * fmSynth->carrierFreq);
  float t = fmSynth->phase;
  float synthSample = sinf(2.0f * PI * t) * fmSynth->volume;
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

  for (ma_uint32 i = 0; i < frameCount; i++) {
    sample = 0.0f; // Reset sample for mixing

    for (int j = 0; j < MAX_INSTRUMENTS; j++) {
      FMSynth *fmSynth = &Instruments[j];
      float *modPhase = &modPhases[j];

      lead_synth_callback(&sample, i, fmSynth, modPhase);
    }

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

  init_audioControls(&audioControls);

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

  audioControls.time += GetFrameTime();

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
