

#include "core.h"

int main(void) {
  core_init_window("Synth");
  while (!core_window_should_close()) {
    core_execute_loop();
  }
  core_close_window();
  return 0;
}
