#pragma once

#include "utils.h"

bool core_init_window(const char *title);

bool core_window_should_close();

void core_execute_loop();

void core_close_window();
