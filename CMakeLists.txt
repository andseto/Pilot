cmake_minimum_required(VERSION 3.20)
project(VoiceToText CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(VoiceToText
    src/main.cpp
    src/SpeechToText/ElevenLabsSttClient.cpp
    src/SpeechToText/AudioCapture.cpp
    src/SpeechToText/Base64.cpp
)

find_package(ixwebsocket CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)

target_link_libraries(VoiceToText PRIVATE
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
    portaudio
)

if (WIN32)
    target_link_libraries(VoiceToText PRIVATE bcrypt)
endif()

if (WIN32)
    target_link_libraries(VoiceToText PRIVATE bcrypt ws2_32)
endif()
