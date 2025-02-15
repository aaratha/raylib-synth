

#include "core.h"

int main(void) {
  core_init_window(1280, 720, "Test"); // 1280x720 window named "Test"
  while (!core_window_should_close()) {
    core_execute_loop();
  }
  core_close_window();
  return 0;
}
