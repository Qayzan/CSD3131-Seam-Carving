# CSD3131 Assignment 2 — Seam Carving (Team T11)
#
# Build (Visual Studio Developer PowerShell, or any terminal with CMake + a C++17 compiler):
#     cmake -S . -B build
#     cmake --build build --config Release
#     ctest --test-dir build -C Release --output-on-failure
#
# Every .cpp in src/seam_carving/ goes into one library. Every tests/test_*.cpp becomes its
# own test program (it needs its own main()) and is registered with CTest. New files are
# picked up automatically: just add them and re-run the build.
cmake_minimum_required(VERSION 3.16)
project(SeamCarving LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)  # graph cut is slow without optimisation
endif()

if(MSVC)
    set(SEAM_WARNINGS /W4 /permissive- /utf-8)
else()
    set(SEAM_WARNINGS -Wall -Wextra -Wpedantic)
endif()

# ---- The seam carving library (all team members' algorithm code) ----------
file(GLOB SEAM_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/seam_carving/*.cpp)
add_library(seam_carving STATIC ${SEAM_SOURCES})
target_include_directories(seam_carving PUBLIC ${CMAKE_SOURCE_DIR}/src/seam_carving)
target_compile_options(seam_carving PRIVATE ${SEAM_WARNINGS})

# ---- The demo application (shared foundation, added later as src/main.cpp) --
if(EXISTS ${CMAKE_SOURCE_DIR}/src/main.cpp)
    add_executable(seam_carver ${CMAKE_SOURCE_DIR}/src/main.cpp)
    target_link_libraries(seam_carver PRIVATE seam_carving)
    target_compile_options(seam_carver PRIVATE ${SEAM_WARNINGS})
    # Visual Studio solution (open_in_visual_studio.bat): F5 runs the demo on the
    # sample picture from the repository folder, instead of the "ALL_BUILD" project.
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT seam_carver)
    set_target_properties(seam_carver PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "assets/images/synthetic_scene.png examples/output/vs/carved.png --width 336 --seams examples/output/vs/seams.png")
endif()

# ---- Tests: one program per tests/test_*.cpp --------------------------------
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE seam_carving)
    target_compile_options(${test_name} PRIVATE ${SEAM_WARNINGS})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
