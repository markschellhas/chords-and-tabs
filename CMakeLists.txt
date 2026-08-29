cmake_minimum_required(VERSION 3.22)

project(chords_and_tabs VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CHORDS_BUILD_TESTS "Build unit tests" ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
include_directories(${FREETYPE_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS})
add_compile_options(${FREETYPE_CFLAGS_OTHER} ${FONTCONFIG_CFLAGS_OTHER})

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
set(JUCE_DIR "")
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/JUCE/CMakeLists.txt")
    set(JUCE_DIR "${CMAKE_SOURCE_DIR}/libs/JUCE")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../herman-band/build/_deps/juce-src/CMakeLists.txt")
    set(JUCE_DIR "${CMAKE_SOURCE_DIR}/../herman-band/build/_deps/juce-src")
endif()

if(JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(juce)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CHORDS_AUDIO_BACKEND_DEFS JUCE_JACK=1 JUCE_ALSA=1)
else()
    set(CHORDS_AUDIO_BACKEND_DEFS JUCE_JACK=0 JUCE_ALSA=0)
endif()

# ---------------------------------------------------------------------------
# Core (no GUI) — theory, song, timeline, agent API
# ---------------------------------------------------------------------------
add_library(chords_core STATIC
    src/theory/MusicTheory.cpp
    src/model/Song.cpp
    src/model/Timeline.cpp
    src/api/SongJson.cpp
    src/api/AgentHttpServer.cpp
    src/api/AgentClient.cpp
)
target_include_directories(chords_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
find_package(Threads REQUIRED)
target_link_libraries(chords_core PUBLIC Threads::Threads)

# ---------------------------------------------------------------------------
# Agent CLI (no GUI) — any command-running agent can call this
# ---------------------------------------------------------------------------
add_executable(chords-agent src/cli/chords-agent.cpp)
target_link_libraries(chords-agent PRIVATE chords_core)

# ---------------------------------------------------------------------------
# GUI application
# ---------------------------------------------------------------------------
juce_add_gui_app(chords_and_tabs
    PRODUCT_NAME "chords-and-tabs"
    COMPANY_NAME "chords-and-tabs"
    BUNDLE_ID "org.chordsandtabs.app"
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
)

target_sources(chords_and_tabs
    PRIVATE
        src/main.cpp
        src/MainComponent.cpp
        src/audio/ChordEngine.cpp
        src/gui/AppLookAndFeel.cpp
        src/gui/CircleOfFifthsComponent.cpp
        src/gui/ChordSlotComponent.cpp
        src/gui/MeasureComponent.cpp
        src/gui/SectionComponent.cpp
        src/gui/SectionListComponent.cpp
        src/gui/PianoKeyboard.cpp
        src/gui/TransportStrip.cpp
)

target_include_directories(chords_and_tabs PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(chords_and_tabs
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        ${CHORDS_AUDIO_BACKEND_DEFS}
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(chords_and_tabs
    PRIVATE
        chords_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(CHORDS_BUILD_TESTS)
    enable_testing()

    add_executable(MusicTheoryTest tests/MusicTheoryTest.cpp)
    target_link_libraries(MusicTheoryTest PRIVATE chords_core)
    add_test(NAME MusicTheoryTest COMMAND MusicTheoryTest)

    add_executable(SongModelTest tests/SongModelTest.cpp)
    target_link_libraries(SongModelTest PRIVATE chords_core)
    add_test(NAME SongModelTest COMMAND SongModelTest)

    add_executable(AgentApiTest tests/AgentApiTest.cpp)
    target_link_libraries(AgentApiTest PRIVATE chords_core)
    add_test(NAME AgentApiTest COMMAND AgentApiTest)

    add_executable(InstrumentsTest tests/InstrumentsTest.cpp)
    target_link_libraries(InstrumentsTest PRIVATE chords_core)
    add_test(NAME InstrumentsTest COMMAND InstrumentsTest)
endif()

if(APPLE)
    install(TARGETS chords_and_tabs BUNDLE DESTINATION "." RUNTIME DESTINATION bin)
    install(TARGETS chords-agent RUNTIME DESTINATION bin)
else()
    install(TARGETS chords_and_tabs chords-agent RUNTIME DESTINATION bin)
endif()
