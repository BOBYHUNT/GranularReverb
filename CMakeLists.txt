cmake_minimum_required(VERSION 3.22)
project(GranularReverb VERSION 1.0.0)

add_subdirectory(JUCE)

juce_add_plugin(GranularReverb
    COMPANY_NAME "MonStudio"
    BUNDLE_ID "com.monstudio.GranularReverb"
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE MStu
    PLUGIN_CODE GrVb
    FORMATS VST3 Standalone
    PRODUCT_NAME "Granular Reverb")

juce_generate_juce_header(GranularReverb)

target_sources(GranularReverb PRIVATE
    Source/GranularReverbProcessor.cpp
    Source/GranularReverbEditor.cpp)

target_compile_definitions(GranularReverb PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GranularReverb PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

if(NOT WIN32)
    target_link_libraries(GranularReverb PRIVATE
        X11 Xext Xrandr Xcomposite Xcursor Xinerama Xi)
endif()
