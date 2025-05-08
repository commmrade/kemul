cmake_minimum_required(VERSION 3.16)
project(proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing() # Поддержка тестов

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(GTest REQUIRED)

pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

add_executable(tests tests/test.cpp tests/terminal_test.cpp src/Buffer.cpp)

add_executable(proj
    src/main.cpp
    src/Application.cpp
    src/Window.cpp
    src/Buffer.cpp
    src/EventHandler.cpp
    src/GlyphCache.cpp
    src/ANSIParser.cpp
)

target_link_libraries(proj
    SDL2::SDL2
    ${SDL2_TTF_LIBRARIES}
    -licuuc
)
target_link_libraries(tests
    SDL2::SDL2
    ${SDL2_TTF_LIBRARIES}
)
target_link_libraries(tests GTest::gtest GTest::gtest_main)
target_include_directories(proj PRIVATE
    ${SDL2_TTF_INCLUDE_DIRS}
)
target_include_directories(tests PRIVATE
    ${SDL2_TTF_INCLUDE_DIRS}
)
