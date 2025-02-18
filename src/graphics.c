#include "graphics.h"
#include "synth.h"

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
