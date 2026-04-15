cmake_minimum_required(VERSION 3.20)
project(DoomViz VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Build the Doom renderer library
add_subdirectory(libs/doom_renderer)

# --- Dependencies ---
add_subdirectory(extern/JUCE)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/yaml-cpp)

juce_add_plugin(DoomViz
    COMPANY_NAME "DoomViz"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE DmVz
    PLUGIN_CODE DmV1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "DoomViz"
)

target_sources(DoomViz PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/DoomViewport.cpp
    src/doom/DoomEngine.cpp
    src/audio/SignalBus.cpp
    src/audio/AudioAnalyzer.cpp
    src/audio/MidiHandler.cpp
    src/routing/RouteConfig.cpp
    src/routing/SignalRouter.cpp
)

target_include_directories(DoomViz PRIVATE
    src
    libs/doom_renderer/include
    extern/yaml-cpp/include
)

target_compile_definitions(DoomViz PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(DoomViz PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_opengl
    juce::juce_dsp
    doom_renderer
    yaml-cpp
)

# Copy DOOM1.WAD into each plugin bundle's Resources after build
set(WAD_SRC "${CMAKE_SOURCE_DIR}/resources/DOOM1.WAD")
foreach(target DoomViz_VST3 DoomViz_AU DoomViz_Standalone)
    if(TARGET ${target})
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:${target}>/Contents/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WAD_SRC}"
                "$<TARGET_BUNDLE_DIR:${target}>/Contents/Resources/DOOM1.WAD"
            COMMENT "Bundling DOOM1.WAD into ${target}"
        )
    endif()
endforeach()

# --- Test harness (standalone C program, not the plugin) ---
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
