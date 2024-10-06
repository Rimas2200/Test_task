cmake_minimum_required(VERSION 3.1)

set(name Liveness_Detection)

project(${name})

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(${name} main.cpp)

set_property(TARGET ${name} PROPERTY CXX_STANDARD 17)

target_link_libraries(${name} ${OpenCV_LIBS})

install(TARGETS ${name} DESTINATION bin)
