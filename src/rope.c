#include "rope.h"

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
