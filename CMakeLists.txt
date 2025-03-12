cmake_minimum_required(VERSION 3.10)
project(rl_synth C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


# Execute miniaudio setup script
execute_process(
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/setup_miniaudio.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Include FetchContent module
include(FetchContent)

# Set the base directory for FetchContent to the 'external' directory
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/external)

# Declare raylib as a dependency
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5  # Specify the desired version
)

# Prevent building examples and tests for raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

# Download and add raylib to the build
FetchContent_MakeAvailable(raylib)

# Add source files
add_executable(${PROJECT_NAME} src/main.c src/core.c src/rope.c src/utils.c src/synth.c src/graphics.c)
target_link_libraries(${PROJECT_NAME} raylib)

# Add miniaudio include directory
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/miniaudio ${CMAKE_SOURCE_DIR}/include)

# Platform-specific settings
if(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework CoreAudio" "-framework AudioToolbox")
endif()


if(EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 --shell-file ${CMAKE_SOURCE_DIR}/shell.html")  # Add this line
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # Set executable to build with the Emscripten HTML template
    # Add these lines to rename the output file
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # Explicitly set the output file name
    set(CMAKE_EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/index.html")
endif()
