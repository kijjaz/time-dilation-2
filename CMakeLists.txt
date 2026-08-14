cmake_minimum_required(VERSION 3.15)
project(TimeDilationDAW VERSION 0.0.3 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
)
FetchContent_MakeAvailable(juce)

juce_add_gui_app(TimeDilationDAW
    PRODUCT_NAME "Time Dilation DAW"
    VERSION 0.0.3
)

target_sources(TimeDilationDAW
    PRIVATE
        Source/Main.cpp
        Source/dsp/TimePolyFrame.h
        Source/dsp/RelativisticNodeGraph.h
        Source/dsp/RelativisticNodeGraph.cpp
        Source/dsp/RelativisticNodeFactory.h
        Source/dsp/RelativisticNodeFactory.cpp
        Source/dsp/RelativisticSoundNodes.h
        Source/dsp/RelativisticSoundNodes.cpp
        Source/dsp/RelativisticSequencers.h
        Source/dsp/RelativisticSequencers.cpp
        Source/dsp/PdControlNodes.h
        Source/dsp/PdControlNodes.cpp
        Source/dsp/PdSampleNodes.h
        Source/dsp/PdSampleNodes.cpp
        Source/dsp/PdDelayNodes.h
        Source/dsp/PdDelayNodes.cpp
        Source/dsp/CompositeNode.h
        Source/dsp/CompositeNode.cpp
        Source/dsp/PrintNode.h
        Source/dsp/PrintNode.cpp
        Source/gui/CarbonGoldLookAndFeel.h
        Source/gui/CarbonGoldLookAndFeel.cpp
        Source/gui/RelativisticCanvasComponent.h
        Source/gui/RelativisticCanvasComponent.cpp
        Source/gui/OscilloscopeComponent.h
        Source/gui/OscilloscopeComponent.cpp
        Source/gui/NodeInspectorComponent.h
        Source/gui/NodeInspectorComponent.cpp
        Source/gui/TrackViewComponent.h
        Source/gui/TrackViewComponent.cpp
        Source/gui/ArrangementTimelineComponent.h
        Source/gui/ArrangementTimelineComponent.cpp
        Source/gui/ConsolePanelComponent.h
        Source/gui/ConsolePanelComponent.cpp
        Source/gui/WorkstationContainerComponent.h
        Source/gui/WorkstationContainerComponent.cpp
        Source/utils/ConsoleLogger.h
        Source/utils/AgentTestRunner.h
        Source/utils/AgentTestRunner.cpp
        Source/utils/TerminalCommandProcessor.h
        Source/utils/TerminalCommandProcessor.cpp
)

target_compile_definitions(TimeDilationDAW
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(TimeDilationDAW
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
