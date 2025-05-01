cmake_minimum_required(VERSION 3.16)
project(proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

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
    ${HARFBUZZ_LIBRARIES}
    -licuuc
)

target_include_directories(proj PRIVATE
    ${SDL2_TTF_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
)
