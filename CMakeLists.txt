cmake_minimum_required(VERSION 3.22)

project(DiscNativePro VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch JUCE 8
message(STATUS "Fetching JUCE 8 framework...")
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(DiscNativePro
    PRODUCT_NAME "DiscNativePro"
    COMPANY_NAME "DiscPro"
    BUNDLE_ID "com.discpro.discnativepro"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
)

target_sources(DiscNativePro
    PRIVATE
        src/Main.cpp
        src/audio/DeckPlayer.h
        src/audio/DeckPlayer.cpp
        src/audio/DjMixer.h
        src/audio/DjMixer.cpp
        src/audio/BeatDetector.h
        src/audio/BeatDetector.cpp
        src/audio/AudioEngine.h
        src/audio/AudioEngine.cpp
        src/audio/AudioRecorder.h
        src/audio/AudioRecorder.cpp
        src/audio/SampleTrackGenerator.h
        src/audio/SampleTrackGenerator.cpp
        src/midi/MidiManager.h
        src/midi/MidiManager.cpp
        src/services/YouTubeService.h
        src/services/YouTubeService.cpp
        src/services/ThumbnailCache.h
        src/services/ThumbnailCache.cpp
        src/ui/TopBarComponent.h
        src/ui/TopBarComponent.cpp
        src/ui/DjKnob.h
        src/ui/VuMeter.h
        src/ui/JogWheel.h
        src/ui/JogWheel.cpp
        src/ui/OverviewWaveform.h
        src/ui/OverviewWaveform.cpp
        src/ui/VerticalWaveform.h
        src/ui/VerticalWaveform.cpp
        src/ui/ChannelStripComponent.h
        src/ui/ChannelStripComponent.cpp
        src/ui/CenterColumnComponent.h
        src/ui/CenterColumnComponent.cpp
        src/ui/ScrollingWaveform.h
        src/ui/ScrollingWaveform.cpp
        src/ui/DeckComponent.h
        src/ui/DeckComponent.cpp
        src/ui/YouTubeVideoComponent.h
        src/ui/MixerComponent.h
        src/ui/MixerComponent.cpp
        src/ui/LibraryComponent.h
        src/ui/LibraryComponent.cpp
        src/ui/YouTubeLibraryView.h
        src/ui/YouTubeLibraryView.cpp
        src/ui/FolderTreeComponent.h
        src/ui/FolderTreeComponent.cpp
        src/ui/MidiModalComponent.h
        src/ui/MidiModalComponent.cpp
        src/ui/SettingsModalComponent.h
        src/ui/SettingsModalComponent.cpp
        src/ui/FxPanelComponent.h
        src/ui/FxPanelComponent.cpp
        src/ui/MainComponent.h
        src/ui/MainComponent.cpp
        src/ui/CoverArtGenerator.h
        src/ui/CoverArtGenerator.cpp
        src/ui/DjLookAndFeel.h
        src/ui/DjLookAndFeel.cpp
        src/ui/LucideIcons.h
        src/ui/LucideIcons.cpp
        src/ui/DjButton.h
)

target_include_directories(DiscNativePro
    PRIVATE
        src
)

target_link_libraries(DiscNativePro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

if(APPLE)
    target_sources(DiscNativePro
        PRIVATE
            src/ui/YouTubeVideoComponent.mm
    )
    target_compile_definitions(DiscNativePro
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
    )
    target_link_libraries(DiscNativePro
        PRIVATE
            "-framework WebKit"
            "-framework Cocoa"
    )
elseif(WIN32)
    list(APPEND CMAKE_MODULE_PATH
        "${juce_SOURCE_DIR}/extras/Build/CMake"
        "${JUCE_SOURCE_DIR}/extras/Build/CMake"
    )
    find_package(WebView2 QUIET)
    target_sources(DiscNativePro
        PRIVATE
            src/ui/YouTubeVideoComponent_win.cpp
    )
    target_compile_definitions(DiscNativePro
        PRIVATE
            JUCE_WEB_BROWSER=1
            JUCE_USE_WIN_WEBVIEW2=1
            JUCE_USE_CURL=0
    )
    target_link_libraries(DiscNativePro
        PRIVATE
            psapi
    )
    if(TARGET juce::juce_webview2)
        target_link_libraries(DiscNativePro PRIVATE juce::juce_webview2)
    endif()

    # Fallback directo en carpeta packages
    file(GLOB_RECURSE WEBVIEW2_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/packages/*/WebView2.h")
    if(WEBVIEW2_HEADERS)
        list(GET WEBVIEW2_HEADERS 0 FIRST_WV2_HEADER)
        get_filename_component(WV2_INC_DIR "${FIRST_WV2_HEADER}" DIRECTORY)
        target_include_directories(DiscNativePro PRIVATE "${WV2_INC_DIR}")
    endif()
    file(GLOB_RECURSE WV2_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/packages/*/x64/WebView2LoaderStatic.lib")
    if(WV2_LIBS)
        list(GET WV2_LIBS 0 FIRST_WV2_LIB)
        target_link_libraries(DiscNativePro PRIVATE "${FIRST_WV2_LIB}")
    endif()
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    pkg_search_module(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1 webkit2gtk-4.0)

    target_sources(DiscNativePro
        PRIVATE
            src/ui/YouTubeVideoComponent_linux.cpp
    )
    target_compile_definitions(DiscNativePro
        PRIVATE
            JUCE_WEB_BROWSER=1
            JUCE_USE_CURL=0
    )
    target_link_libraries(DiscNativePro
        PRIVATE
            PkgConfig::GTK3
            PkgConfig::WEBKIT
    )
endif()
