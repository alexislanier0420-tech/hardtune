set(HARDTUNE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND HARDTUNE_FORMATS AU)
endif()

juce_add_plugin(HardTune
    COMPANY_NAME "Alexis McGill"
    PLUGIN_MANUFACTURER_CODE Almc   # four characters, at least one upper-case
    PLUGIN_CODE Htn1                # four characters, exactly one upper-case
    FORMATS ${HARDTUNE_FORMATS}
    PRODUCT_NAME "HardTune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(HardTune
    PRIVATE
        PluginProcessor.cpp)

target_compile_definitions(HardTune
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(HardTune
    PRIVATE
        hardtune_dsp
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
