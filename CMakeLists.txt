cmake_minimum_required(VERSION 3.22)
project(DeessPocket VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(deess_core STATIC Source/DSP/DeessEngine.cpp)
target_include_directories(deess_core PUBLIC Source/DSP)
set_target_properties(deess_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_executable(deess_render Tools/OfflineRender.cpp)
target_link_libraries(deess_render PRIVATE deess_core)

option(DEESS_BUILD_TESTS "Build DSP tests" ON)
if(DEESS_BUILD_TESTS)
    enable_testing()
    add_executable(deess_core_tests Tests/CoreTests.cpp)
    target_link_libraries(deess_core_tests PRIVATE deess_core)
    add_test(NAME deess_core_tests COMMAND deess_core_tests)
endif()

option(DEESS_BUILD_PLUGIN "Build VST3/standalone with JUCE" OFF)
if(DEESS_BUILD_PLUGIN)
    if(DEFINED JUCE_DIR)
        add_subdirectory("${JUCE_DIR}" JUCE)
    else()
        find_package(JUCE CONFIG REQUIRED)
    endif()
    juce_add_binary_data(DeessArtwork SOURCES Design/Assets/background.png Design/Assets/dial-face.png)
    set(DEESS_FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND DEESS_FORMATS AU)
    endif()
    if(DEFINED AAX_SDK_PATH)
        list(APPEND DEESS_FORMATS AAX)
    endif()
    juce_add_plugin(DeessPocket
        COMPANY_NAME "Rainline Music"
        BUNDLE_ID "ru.rainlinemusic.deesspocket"
        PLUGIN_MANUFACTURER_CODE RnLn
        PLUGIN_CODE DsPk
        FORMATS ${DEESS_FORMATS}
        PRODUCT_NAME "Deess Pocket")
    target_sources(DeessPocket PRIVATE Source/Plugin/PluginProcessor.cpp Source/Plugin/PluginEditor.cpp)
    target_link_libraries(DeessPocket PRIVATE deess_core DeessArtwork juce::juce_audio_utils juce::juce_dsp)
    target_compile_definitions(DeessPocket PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    juce_generate_juce_header(DeessPocket)
endif()
