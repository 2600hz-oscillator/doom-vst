cmake_minimum_required(VERSION 3.20)
project(doom_renderer C)

# Collect doom source files, excluding ones we don't need
file(GLOB DOOM_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/doom_src/*.c")
# m_misc.c has portability issues (string-to-int casts in static initializers)
# and provides config file loading we don't need
list(FILTER DOOM_SRC_FILES EXCLUDE REGEX "m_misc\\.c$")

# Stub files
set(STUB_FILES
    src/doom_renderer.c
    src/i_video_stub.c
    src/i_system_stub.c
    src/i_sound_stub.c
    src/d_net_stub.c
    src/game_stubs.c
)

# Build as static library
add_library(doom_renderer STATIC ${DOOM_SRC_FILES} ${STUB_FILES})

# Include paths
target_include_directories(doom_renderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/doom_src
)

# Compile definitions
target_compile_definitions(doom_renderer PRIVATE
    NORMALUNIX
    DOOMVIZ
)

# C standard. POSITION_INDEPENDENT_CODE is required so this static lib
# can be linked into the JUCE plugin shared module (DoomViz.so on Linux,
# DoomViz.vst3/Contents/MacOS/DoomViz on macOS). Apple's ld handles this
# automatically, but GNU ld errors with "relocation R_X86_64_PC32 against
# symbol ... can not be used when making a shared object" without it.
set_target_properties(doom_renderer PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE ON
)

# Suppress warnings from vintage 1997 C code. The flags are GNU/Clang style;
# MSVC's cl.exe rejects them with D8021. MSVC's default /W3 is lenient
# enough that this code compiles without explicit suppressions there — if
# specific /wd<N> codes turn out to be needed later, add them under the
# else() branch.
if(NOT MSVC)
    target_compile_options(doom_renderer PRIVATE
        -Wno-implicit-function-declaration
        -Wno-implicit-int
        -Wno-int-conversion
        -Wno-pointer-sign
        -Wno-unused-result
        -Wno-format
        -Wno-incompatible-pointer-types
        -Wno-parentheses
        -Wno-unused-variable
        -Wno-missing-braces
    )
endif()

# Link math library. Windows' MSVC links libm implicitly via the CRT;
# only Unix-style toolchains need an explicit -lm.
if(NOT MSVC)
    target_link_libraries(doom_renderer PRIVATE m)
endif()
