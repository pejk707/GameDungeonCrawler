cmake_minimum_required(VERSION 3.16)
project(lamplighter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive- /EHsc)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Вся логика игры — в статической библиотеке, чтобы её могли использовать и игра, и тесты.
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/src/app/.*")
add_library(lamplighter_core STATIC ${CORE_SOURCES})
target_include_directories(lamplighter_core PUBLIC src third_party)

add_executable(lamplighter src/app/main.cpp)
target_link_libraries(lamplighter PRIVATE lamplighter_core)

# Данные и ASCII-арт копируются рядом с исполняемым файлом.
add_custom_command(TARGET lamplighter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:lamplighter>/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/art $<TARGET_FILE_DIR:lamplighter>/art)

option(LAMPLIGHTER_TESTS "Собирать модульные тесты" ON)
if(LAMPLIGHTER_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
    add_executable(lamplighter_tests ${TEST_SOURCES})
    target_link_libraries(lamplighter_tests PRIVATE lamplighter_core)
    target_compile_definitions(lamplighter_tests PRIVATE LAMPLIGHTER_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME lamplighter_tests COMMAND lamplighter_tests)
endif()
