cmake_minimum_required(VERSION 3.22)
project(GPUNEQ VERSION 1.0.0)

# Set JUCE_DIR to your JUCE checkout, or pass -DJUCE_DIR=...
if(NOT DEFINED JUCE_DIR)
    message(FATAL_ERROR "Set JUCE_DIR to your JUCE folder, e.g. -DJUCE_DIR=C:/JUCE")
endif()
add_subdirectory("${JUCE_DIR}" JUCE)

juce_add_plugin(GPUNEQ
    COMPANY_NAME "GPUN Audio"
    BUNDLE_ID "com.gpunaudio.gpuneq"
    PLUGIN_MANUFACTURER_CODE GPUN
    PLUGIN_CODE InEQ
    FORMATS VST3 Standalone
    PRODUCT_NAME "GPUN EQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(GPUNEQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)
target_compile_features(GPUNEQ PRIVATE cxx_std_17)
target_compile_definitions(GPUNEQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(GPUNEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
