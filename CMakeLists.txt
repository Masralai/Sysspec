cmake_minimum_required(VERSION 3.10)

project(Sysspec VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES src/main.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(Sysspec ${SOURCES})

if(WIN32)
    target_link_libraries(Sysspec PRIVATE Advapi32 User32)
endif()
