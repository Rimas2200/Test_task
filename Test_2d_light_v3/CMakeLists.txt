cmake_minimum_required(VERSION 3.1)

set(name Test_2d_light_v3)

project(${name})

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/libfacerec/include)

add_executable(${name} main.cpp)

set_property(TARGET ${name} PROPERTY CXX_STANDARD 17)

set(LIBS pbio_cpp ${OpenCV_LIBS})
target_link_libraries(${name} ${LIBS})

if(WIN32)
    target_link_libraries(${name} ws2_32)
    target_link_libraries(${name} pthread)
endif()

link_directories(${CMAKE_SOURCE_DIR}/libfacerec/bin)

install(TARGETS ${name} DESTINATION bin)
