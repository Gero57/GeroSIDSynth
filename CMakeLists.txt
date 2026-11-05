cmake_minimum_required(VERSION 3.22)

project(GeroSIDSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GeroSIDSynth
    COMPANY_NAME "Gero"
    BUNDLE_ID "it.gero.gerosidsynth"
    PLUGIN_MANUFACTURER_CODE Gero
    PLUGIN_CODE Gsid
    FORMATS VST3
    PRODUCT_NAME "Gero SID Synth"
    DESCRIPTION "Commodore SID 6581 / 8580 emulation instrument"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(GeroSIDSynth)

target_sources(GeroSIDSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_compile_definitions(GeroSIDSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GeroSIDSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
