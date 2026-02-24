cmake_minimum_required(VERSION 3.22)
project(OBSTACLE VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Download JUCE ─────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(OBSTACLE
    PLUGIN_MANUFACTURER_CODE  Fred
    PLUGIN_CODE               Obst
    FORMATS                   AU Standalone
    PRODUCT_NAME              "OBSTACLE"
    COMPANY_NAME              "Fred"
    BUNDLE_ID                 "com.fred.obstacle"
    PLUGIN_MANUFACTURER_NAME  "Fred"
    AU_MAIN_TYPE              "kAudioUnitType_MusicDevice"
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

juce_generate_juce_header(OBSTACLE)

target_sources(OBSTACLE PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(OBSTACLE PRIVATE Source)

target_compile_definitions(OBSTACLE PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OBSTACLE PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
