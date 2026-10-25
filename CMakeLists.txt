cmake_minimum_required(VERSION 3.22)
project(Acid303 VERSION 0.2.0)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(Acid303
  COMPANY_NAME "G"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  PLUGIN_MANUFACTURER_CODE Gprd
  PLUGIN_CODE A303
  FORMATS ${PLUGIN_FORMATS}
  PRODUCT_NAME "Acid303")

target_sources(Acid303 PRIVATE
  Source/PluginProcessor.h
  Source/PluginProcessor.cpp
  Source/PluginEditor.h
  Source/PluginEditor.cpp)

target_compile_features(Acid303 PRIVATE cxx_std_17)
target_compile_definitions(Acid303 PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Acid303
  PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
