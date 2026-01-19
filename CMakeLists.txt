cmake_minimum_required(VERSION 3.20)
project(VoiceToText CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(VoiceToText
    src/main.cpp
    src/SpeechToText/ElevenLabsSttClient.cpp
    src/SpeechToText/AudioCapture.cpp
    src/SpeechToText/Base64.cpp
    src/ChatGPT/ChatGPT.cpp
)

find_package(ixwebsocket CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

target_link_libraries(VoiceToText PRIVATE
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
    portaudio
    cpr::cpr
)

# So you can do: #include "ChatGPT/ChatGPT.h"
target_include_directories(VoiceToText PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (WIN32)
    target_link_libraries(VoiceToText PRIVATE bcrypt ws2_32)
endif()
