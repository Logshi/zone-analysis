cmake_minimum_required(VERSION 3.16)
project(rtsp_dwell_alert LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(dwell_alert
    src/main.cpp
    src/camera.cpp
    src/detector.cpp
    src/tracker.cpp
    src/notifier.cpp
)

target_include_directories(dwell_alert PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(dwell_alert PRIVATE ${OpenCV_LIBS})

install(TARGETS dwell_alert RUNTIME DESTINATION bin)
