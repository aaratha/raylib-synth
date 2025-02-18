#pragma once

#include "utils.h"

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

void init_rope(Rope *rope, vec2 start, vec2 end, Color color);
void solve_rope_constraints(Rope *rope);
void update_rope(Rope *rope);
void draw_rope(Rope *rope);
