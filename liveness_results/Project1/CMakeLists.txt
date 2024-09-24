cmake_minimum_required(VERSION 8.2)
project(LivenessDetection)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

add_executable(liveness_processing FileName.cpp)
target_link_libraries(test_liveness_processing ${OpenCV_LIBS})
