cmake_minimum_required(VERSION 3.15)
project(WAVES VERSION 0.0.1)

add_subdirectory(./build/JUCE ../build-juce)

juce_add_console_app(Waves PRODUCT_NAME "Waves")

target_compile_options(Waves
    PRIVATE
        -Wno-zero-as-null-pointer-constant
        -Wno-deprecated-dynamic-exception-spec
        -Wno-shadow
)

juce_generate_juce_header(Waves)

target_sources(Waves PRIVATE
    Config.cpp
    ConfigParser.cpp
    LiquidPusher.cpp
    Maethstro.cpp
    Main.cpp
    NetworkService.cpp
    Vst3Plugin.cpp
    Renderer.cpp
    SamplerPlugin.cpp
    Shouter.cpp
    Status.cpp
    Track.cpp
    Utils.cpp
    VorbisWriter.cpp
)

target_compile_definitions(Waves
    PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_include_directories(Waves
    PRIVATE
        /usr/local/include
)

target_link_directories(Waves
    PRIVATE
        /usr/local/lib
)

target_link_libraries(Waves
    PRIVATE
        juce::juce_audio_utils
	juce::juce_dsp
        juce::juce_core
	juce::juce_audio_basics
	juce::juce_osc
        yaml-cpp
        glog
        gflags
        shout
        sfml-network
        sfml-system
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
