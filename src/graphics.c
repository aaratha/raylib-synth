#include "graphics.h"
#include "rope.h"
#include "synth.h"
#include "utils.h"

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

static float radius = 100.0f;
static float separation = 30.0f;
void draw_circular_waveforms() {
  // Circle parameters
  Vector2 center = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
  float thickness = 2.0f; // Thickness of the waveform

  float rope_length = Vector2Distance(rope.end, rope.start);
  float targetRadius = lerp1D(MIN_WAVEFORM_RADIUS, MAX_WAVEFORM_RADIUS,
                              rope_length / MAX_ROPE_LENGTH);
  float target_separation =
      lerp1D(MIN_WAVEFORM_SEPARATION, MAX_WAVEFORM_SEPARATION,
             rope_length / MAX_ROPE_LENGTH);

  radius = lerp1D(radius, targetRadius, GRAPHICS_LERP_SPEED * GetFrameTime());
  separation = lerp1D(separation, target_separation,
                      GRAPHICS_LERP_SPEED * GetFrameTime());

  float minBrightness = 100;
  float maxBrightness = 200;

  for (int s = 0; s < MAX_INSTRUMENTS; s++) {
    float brightness =
        (s / (float)MAX_INSTRUMENTS) * (maxBrightness - minBrightness) +
        minBrightness;
    Color color = (Color){brightness, brightness, brightness, 255};
    float specificRadius = radius + s * separation;
    Vector2 prevPoint = {center.x + specificRadius,
                         center.y}; // Start with the first calculated point
    Vector2 firstPoint = prevPoint;

    for (int i = 0; i < BUFFER_SIZE; i++) {
      float angle = (i / (float)BUFFER_SIZE) * 2 * PI; // Angle in radians
      float radius =
          specificRadius + Instruments[s].buffer[i] *
                               WAVEFORM_AMPLITUDE_MULTIPLIER; // Modulate radius
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

void draw_note_grid(vec2 rope_start, vec2 rope_end) {
  Color grid_color = (Color){100, 100, 100, 70};

  vec2 direction = Vector2Subtract(rope_end, rope_start);
  float angle = atan2f(direction.y, direction.x);
  float angle_deg = angle * 180 / PI;

  // Pentatonic scale setup
  float step_size = 360.0f / SCALE_SIZE;
  int index = (int)((angle_deg + 180.0f) / step_size) % SCALE_SIZE;
  float frequency = midi_to_freq(pentatonicScale[index]);

  char note[SCALE_SIZE][4] = {"C3", "D3", "F3", "G3", "Af3",
                              "C4", "D4", "F4", "G4", "Af4"};

  // Draw cone boundaries and labels
  for (int i = 0; i < SCALE_SIZE; i++) {
    // Calculate cone boundaries
    float start_angle = i * step_size - 180.0f + step_size * 2;
    float end_angle = (i + 1) * step_size - 180.0f + step_size * 2;

    // Draw cone boundaries
    Vector2 center = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
    float x_dir = cosf(start_angle * DEG2RAD);
    float y_dir = sinf(start_angle * DEG2RAD);
    DrawLineEx((Vector2){center.x + MIN_GRIDLINE_RADIUS * x_dir,
                         center.y + MIN_GRIDLINE_RADIUS * y_dir},
               (Vector2){center.x + MAX_GRIDLINE_RADIUS * x_dir,
                         center.y + MAX_GRIDLINE_RADIUS * y_dir},
               2, grid_color);

    // Position text in the middle of the cone
    float bisector_angle = (start_angle + end_angle) / 2;
    float text_radius = NOTE_DISPLAY_RADIUS; // Position inside the cone

    Vector2 text_pos = {center.x + text_radius * cosf(bisector_angle * DEG2RAD),
                        center.y +
                            text_radius * sinf(bisector_angle * DEG2RAD)};

    // Rotate text to face outward
    float text_rotation = bisector_angle;
    if (bisector_angle > 90 || bisector_angle < -90) {
      text_rotation += 180; // Keep text upright
    }

    // Measure text for proper centering
    int font_size = 20;
    Vector2 text_size = MeasureTextEx(GetFontDefault(), note[i], font_size, 1);

    DrawTextPro(GetFontDefault(), note[i], text_pos,
                (Vector2){text_size.x / 2, text_size.y / 2}, // Center alignment
                0.0, font_size, 1, grid_color);
  }
}
