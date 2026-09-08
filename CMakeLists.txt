cmake_minimum_required(VERSION 3.10)

project(Sysspec VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/platform/common.cpp
)

if(WIN32)
    list(APPEND SOURCES src/platform/win.cpp)
else()
    list(APPEND SOURCES src/platform/linux.cpp)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(Sysspec ${SOURCES})

target_include_directories(Sysspec PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(Sysspec PRIVATE /W4)
else()
    target_compile_options(Sysspec PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_link_libraries(Sysspec PRIVATE Advapi32 User32)
endif()

install(TARGETS Sysspec RUNTIME DESTINATION bin)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(test_utils tests/test_utils.cpp src/platform/common.cpp)
    target_include_directories(test_utils PRIVATE ${CMAKE_SOURCE_DIR}/include)
    if(NOT MSVC)
        target_compile_options(test_utils PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME utils COMMAND test_utils WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    if(WIN32)
        add_executable(test_cli tests/test_cli.cpp)
        target_include_directories(test_cli PRIVATE ${CMAKE_SOURCE_DIR}/include)
        add_test(NAME cli COMMAND test_cli WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()
endif()
