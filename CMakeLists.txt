cmake_minimum_required(VERSION 3.22)
project(BassDistortion VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BassDistortion
    COMPANY_NAME      "TonNom"
    IS_SYNTH          FALSE
    NEEDS_MIDI_INPUT  FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT    FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Tnnm
    PLUGIN_CODE        Bds1
    FORMATS           VST3 AU Standalone
    PRODUCT_NAME      "Bass Distortion"
)

target_sources(BassDistortion PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(BassDistortion PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BassDistortion PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
