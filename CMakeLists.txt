cmake_minimum_required(VERSION 3.16)
project(proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
enable_testing()

# SDL2, SDL2_ttf
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)

# Fetch GTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# Или конкретный релиз:
# URL https://github.com/google/googletest/archive/refs/tags/release-1.14.0.zip

FetchContent_MakeAvailable(googletest)

# ICU
find_package(ICU REQUIRED COMPONENTS uc)

# Основной исполняемый файл
add_executable(proj
    src/main.cpp
    src/Application.cpp
    src/Window.cpp
    src/Buffer.cpp
    src/EventHandler.cpp
    src/GlyphCache.cpp
    src/ANSIParser.cpp
)

target_link_libraries(proj PRIVATE
    SDL2::SDL2
    PkgConfig::SDL2_TTF
    ICU::uc
)

target_include_directories(proj PRIVATE
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_options(proj PRIVATE -Wall -Wextra)

# Тесты
add_executable(tests
    tests/test.cpp
    tests/terminal_test.cpp
    src/Buffer.cpp
)

target_link_libraries(tests PRIVATE
    SDL2::SDL2
    PkgConfig::SDL2_TTF
    gtest
    gtest_main
)

target_include_directories(tests PRIVATE
    ${SDL2_TTF_INCLUDE_DIRS}
)

add_test(NAME AllTests COMMAND tests)
