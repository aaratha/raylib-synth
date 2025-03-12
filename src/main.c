#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

#include "core.h"

int main(void) {
  core_init_window("Synth");
  #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(core_execute_loop, 1000, 1);
  #else
  while (!core_window_should_close()) {
    core_execute_loop();
  }
  #endif
  core_close_window();
  return 0;
}
