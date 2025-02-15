#pragma once

// Shared library export/import macros
#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_CORE
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif
#else
#ifdef BUILDING_CORE
#define CORE_API __attribute__((visibility("default")))
#else
#define CORE_API
#endif
#endif

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <stdbool.h>

typedef Vector2 vec2;

bool core_init_window(int width, int height, const char *title);

bool core_window_should_close();

void core_execute_loop();

void core_close_window();
