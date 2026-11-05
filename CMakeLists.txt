cmake_minimum_required(VERSION 3.22)

project(GeroSIDSynth VERSION 0.2.0 LANGUAGES C CXX)

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

# Cycle-accurate MOS 6581/8580 emulator. Pin an immutable upstream revision.
FetchContent_Declare(
    RESIDFP
    GIT_REPOSITORY https://github.com/libsidplayfp/libresidfp.git
    GIT_TAG c1791383ca843a855e5d4366da6a7277e34e801d
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(RESIDFP)

set(RESIDFP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/residfp-generated")
file(MAKE_DIRECTORY "${RESIDFP_GENERATED_DIR}/residfp")

# The upstream project currently uses Autotools. Generate its two configured
# headers here so the exact upstream source can be built as a CMake sub-target.
set(RESIDFP_BRANCH_HINTS 1)
set(HAVE_BUILTIN_EXPECT 1)
set(RESIDFP_INLINING 1)
set(RESIDFP_INLINE inline)
set(PACKAGE_VERSION "1.3.0a")

configure_file(
    "${residfp_SOURCE_DIR}/src/siddefs-fp.h.in"
    "${RESIDFP_GENERATED_DIR}/siddefs-fp.h"
    @ONLY
)
configure_file(
    "${residfp_SOURCE_DIR}/src/residfp/sidversion.h.in"
    "${RESIDFP_GENERATED_DIR}/residfp/sidversion.h"
    @ONLY
)

file(GLOB RESIDFP_SOURCES CONFIGURE_DEPENDS
    "${residfp_SOURCE_DIR}/src/*.cpp"
    "${residfp_SOURCE_DIR}/src/resample/*.cpp"
    "${residfp_SOURCE_DIR}/src/residfp/*.cpp"
)
list(APPEND RESIDFP_SOURCES "${residfp_SOURCE_DIR}/src/version.cc")

add_library(residfp STATIC ${RESIDFP_SOURCES})
target_include_directories(residfp
    PUBLIC
        "${RESIDFP_GENERATED_DIR}"
        "${residfp_SOURCE_DIR}/src"
)
target_compile_features(residfp PUBLIC cxx_std_17)
target_compile_definitions(residfp PRIVATE
    RESIDFP_INLINE=inline
    RESIDFP_INLINING=1
)
find_package(Threads REQUIRED)
target_link_libraries(residfp PUBLIC Threads::Threads)
set_target_properties(residfp PROPERTIES POSITION_INDEPENDENT_CODE ON)

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
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(GeroSIDSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GeroSIDSynth PRIVATE
    residfp
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
