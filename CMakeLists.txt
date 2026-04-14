cmake_minimum_required(VERSION 3.20)
project(VoiceToText CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# whisper.cpp — local STT for wake word detection
set(WHISPER_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS       OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/whisper.cpp)

add_executable(VoiceToText
    src/main.cpp
    src/SpeechToText/AudioCapture.cpp
    src/SpeechToText/DeepgramSttClient.cpp
    src/SpeechToText/LocalWhisper.cpp
    src/ChatGPT/ChatGPT.cpp
    src/TextToSpeech/DeepgramTTS.cpp
    src/Audio/AudioPlayer.cpp
    src/Commands/Commands.cpp
    src/Memory/ConversationMemory.cpp
)

find_package(ixwebsocket CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

target_link_libraries(VoiceToText PRIVATE
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
    portaudio
    cpr::cpr
    CURL::libcurl
    whisper
)

# So you can do: #include "ChatGPT/ChatGPT.h"
target_include_directories(VoiceToText PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (WIN32)
    target_link_libraries(VoiceToText PRIVATE bcrypt ws2_32 winmm)
endif()
