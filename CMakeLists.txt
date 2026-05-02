cmake_minimum_required(VERSION 3.20)
project(DoomViz VERSION 0.2.0 LANGUAGES C CXX)

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

# Catch2 v3 — only its own headers/lib, no install/CTest scaffolding.
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/Catch2)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/extern/Catch2/extras)
include(Catch)

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
    src/ControlWindow.cpp
    src/doom/DoomEngine.cpp
    src/audio/SignalBus.cpp
    src/audio/AudioAnalyzer.cpp
    src/audio/MidiHandler.cpp
    src/routing/RouteConfig.cpp
    src/routing/SignalRouter.cpp
    src/scenes/SceneManager.cpp
    src/scenes/KillRoomScene.cpp
    src/scenes/AnalyzerRoomScene.cpp
    src/scenes/Spectrum2Scene.cpp
    src/patch/PatchSettingsStore.cpp
    src/patch/PatchWindow.cpp
    src/patch/SpectrumPatchPanel.cpp
    src/patch/PlaceholderPatchPanel.cpp
    $<$<PLATFORM_ID:Darwin>:src/MacFullscreen.mm>
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
set_tests_properties(render_baseline PROPERTIES LABELS "integration")

# --- Unit tests (Catch2) ---
# Console app rather than plain add_executable: JUCE's CMake expects the
# juce_add_* macros to wire up its module compile flags. We compile the
# testable subset of plugin sources directly into the test binary instead
# of linking against DoomViz_SharedCode (which pulls in the full plugin
# client wrappers and editor).
juce_add_console_app(doomviz_unit_tests
    PRODUCT_NAME "DoomViz Unit Tests"
)

target_sources(doomviz_unit_tests PRIVATE
    test/unit/test_signal_bus.cpp
    test/unit/test_audio_analyzer.cpp
    test/unit/test_midi_handler.cpp
    test/unit/test_route_config.cpp
    test/unit/test_signal_router.cpp
    test/unit/test_patch_settings_store.cpp
    test/unit/test_sprite_catalog.cpp
    src/audio/SignalBus.cpp
    src/audio/AudioAnalyzer.cpp
    src/audio/MidiHandler.cpp
    src/routing/RouteConfig.cpp
    src/routing/SignalRouter.cpp
    src/patch/PatchSettingsStore.cpp
)

target_include_directories(doomviz_unit_tests PRIVATE
    src
    extern/yaml-cpp/include
)

target_compile_definitions(doomviz_unit_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
)

target_link_libraries(doomviz_unit_tests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    Catch2::Catch2WithMain
    yaml-cpp
)

catch_discover_tests(doomviz_unit_tests
    PROPERTIES LABELS "unit"
)

# --- Integration tests (Catch2 + real DoomVizProcessor, no editor / OpenGL) ---
juce_add_console_app(doomviz_integration_tests
    PRODUCT_NAME "DoomViz Integration Tests"
)

target_sources(doomviz_integration_tests PRIVATE
    test/integration/test_processor_pass_through.cpp
    test/integration/test_processor_midi.cpp
    test/integration/test_processor_state.cpp
    test/integration/processor_test_stub.cpp
    src/PluginProcessor.cpp
    src/audio/SignalBus.cpp
    src/audio/MidiHandler.cpp
    src/patch/PatchSettingsStore.cpp
)

target_include_directories(doomviz_integration_tests PRIVATE
    src
    extern/yaml-cpp/include
)

target_compile_definitions(doomviz_integration_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
    DOOMVIZ_TEST_BUILD=1
)

target_link_libraries(doomviz_integration_tests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    Catch2::Catch2WithMain
    yaml-cpp
)

catch_discover_tests(doomviz_integration_tests
    PROPERTIES LABELS "integration"
)

# --- VRT runner (no Catch2; standalone tool driven by the Python harness) ---
juce_add_console_app(doomviz_vrt_runner
    PRODUCT_NAME "DoomViz VRT Runner"
)

target_sources(doomviz_vrt_runner PRIVATE
    test/vrt/vrt_runner.cpp
    src/audio/SignalBus.cpp
    src/audio/AudioAnalyzer.cpp
    src/routing/RouteConfig.cpp
    src/routing/SignalRouter.cpp
    src/patch/PatchSettingsStore.cpp
    src/doom/DoomEngine.cpp
    src/scenes/SceneManager.cpp
    src/scenes/KillRoomScene.cpp
    src/scenes/AnalyzerRoomScene.cpp
    src/scenes/Spectrum2Scene.cpp
)

target_include_directories(doomviz_vrt_runner PRIVATE
    src
    libs/doom_renderer/include
    extern/yaml-cpp/include
)

target_compile_definitions(doomviz_vrt_runner PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
)

target_link_libraries(doomviz_vrt_runner PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    doom_renderer
    yaml-cpp
)
