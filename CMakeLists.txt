cmake_minimum_required(VERSION 3.21)

# Some fetched dependencies (e.g. via vcpkg) declare an old
# minimum cmake which newer cmake version refuse unless the floor is manually updated.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file"
)

project(ClassDiagramGenerator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(unofficial-tree-sitter CONFIG REQUIRED)

# ---- Sources shared between the real program and the tests ----

file(GLOB_RECURSE cdgen_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c++"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h++"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# main.cpp defines main() -- it can only go in the real executable.
# The test executable supplies its own main() via doctest.
set(cdgen_SOURCES_NO_MAIN ${cdgen_SOURCES})
list(FILTER cdgen_SOURCES_NO_MAIN EXCLUDE REGEX ".*/main\\.cpp$")

include(FetchContent)

# tree-sitter-cpp has no CMakeLists.txt we want to use directly, so just
# fetch its sources with Populate and build the grammar ourselves below.
FetchContent_Declare(
        tree-sitter-cpp
        GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-cpp.git
        GIT_TAG        v0.23.4
)
cmake_policy(SET CMP0169 OLD) # suppress a warning about deprecated use of FetchContent_Populate
FetchContent_Populate(tree-sitter-cpp)

add_library(tree-sitter-cpp-grammar STATIC
        ${tree-sitter-cpp_SOURCE_DIR}/src/parser.c
        ${tree-sitter-cpp_SOURCE_DIR}/src/scanner.c
)

# ---- Real program ----

add_executable(cdgen)

target_sources(cdgen PRIVATE ${cdgen_SOURCES})

target_include_directories(cdgen PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
        ${tree-sitter-cpp_SOURCE_DIR}/src
)

target_link_libraries(cdgen PRIVATE
        unofficial::tree-sitter::tree-sitter
        tree-sitter-cpp-grammar
)

# ---- Tests ----

file(GLOB_RECURSE cdgen_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
)
list(FILTER cdgen_TEST_SOURCES EXCLUDE REGEX ".*/test/resources/.*")

add_executable(cdgen_tests)

FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.5.3
)
FetchContent_MakeAvailable(doctest)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

target_sources(cdgen_tests PRIVATE
        ${cdgen_TEST_SOURCES}
        ${cdgen_SOURCES_NO_MAIN}
)

target_include_directories(cdgen_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
        ${tree-sitter-cpp_SOURCE_DIR}/src
)

target_link_libraries(cdgen_tests PRIVATE
        unofficial::tree-sitter::tree-sitter
        tree-sitter-cpp-grammar
        doctest::doctest
)

doctest_discover_tests(cdgen_tests)