cmake_minimum_required(VERSION 3.20)
project(DoomViz VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Build the Doom renderer library
add_subdirectory(libs/doom_renderer)

# Build the test harness
add_executable(render_test test/render_test.c)
target_link_libraries(render_test PRIVATE doom_renderer)
target_include_directories(render_test PRIVATE
    libs/doom_renderer/include
)

# Tests
enable_testing()
add_test(
    NAME render_baseline
    COMMAND ${CMAKE_SOURCE_DIR}/test/test_render_baseline.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
