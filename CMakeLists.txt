# SPDX-License-Identifier: AGPL-3.0-or-later
# Doctor Migrana - Tuner  |  Copyright (C) 2026 GuitarFros
cmake_minimum_required(VERSION 3.22)

project(DoctorMigranaTuner VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Copia o plugin ao cartafol do sistema (~/.vst3 en Linux) despois de compilar (desactivado no CI)
option(DMT_COPY_AFTER_BUILD "Copy plugin to the system plugin folder after build" ON)
option(DMT_BUILD_TESTS      "Build the pitch-detector accuracy test"             OFF)
set(DMT_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (leave empty to download)")

# ---------------------------------------------------------------- JUCE
if (DMT_JUCE_DIR)
    add_subdirectory(${DMT_JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.11
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------- artwork
juce_add_binary_data(DMTunerAssets
    SOURCES
        Assets/pedal_base.jpg
        Assets/knob.png)
set_target_properties(DMTunerAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# ---------------------------------------------------------------- plugin
juce_add_plugin(DoctorMigranaTuner
    COMPANY_NAME                "GuitarFros"
    PLUGIN_NAME                 "Doctor Migrana – Tuner"
    PRODUCT_NAME                "Doctor Migrana - Tuner"
    BUNDLE_ID                   com.guitarfros.doctormigranatuner
    PLUGIN_MANUFACTURER_CODE    Gfrs
    PLUGIN_CODE                 Mtun
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Fx Tools
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "Doctor Migrana - Tuner needs the audio input to detect the pitch."
    COPY_PLUGIN_AFTER_BUILD     ${DMT_COPY_AFTER_BUILD})

target_sources(DoctorMigranaTuner PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/TunerAnalyser.cpp
    Source/TunerAnalyser.h
    Source/PitchDetector.h)

target_include_directories(DoctorMigranaTuner PRIVATE Source)

# Necesario en MSVC para o nome con caracteres non ASCII
target_compile_options(DoctorMigranaTuner PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_compile_definitions(DoctorMigranaTuner PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(DoctorMigranaTuner
    PRIVATE
        DMTunerAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------- tests
if (DMT_BUILD_TESTS)
    include(CTest)
    add_executable(test_pitch_detector Tests/test_pitch_detector.cpp)
    target_include_directories(test_pitch_detector PRIVATE Source)
    add_test(NAME pitch_detector_accuracy COMMAND test_pitch_detector)
endif()
