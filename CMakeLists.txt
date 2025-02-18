cmake_minimum_required(VERSION 3.10)
project(trae_synth C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Execute miniaudio setup script
execute_process(
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/setup_miniaudio.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Find raylib package
find_package(raylib REQUIRED)

# Add source files
add_executable(trae_synth src/main.c src/core.c src/rope.c src/utils.c)
target_link_libraries(trae_synth raylib)

# Add miniaudio include directory
target_include_directories(trae_synth PRIVATE ${CMAKE_SOURCE_DIR}/external/miniaudio ${CMAKE_SOURCE_DIR}/include)

# Platform specific settings
if(APPLE)
    target_link_libraries(trae_synth "-framework CoreAudio" "-framework AudioToolbox")
endif()
