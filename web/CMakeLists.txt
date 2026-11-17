# Interactive browser demo: the seam carving library compiled to WebAssembly.
#
# Needs the Emscripten SDK (https://emscripten.org/docs/getting_started/downloads.html):
#     emcmake cmake -S web -B build-web
#     cmake --build build-web
# The finished site is build-web/site/ (open it through a local web server, e.g.
#     python -m http.server -d build-web/site 8000
# and go to http://localhost:8000). Deploy that folder as it is to any static host.
cmake_minimum_required(VERSION 3.16)
project(SeamCarvingWeb LANGUAGES CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this folder with emcmake (Emscripten), see the comment at the top.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SITE_OUT ${CMAKE_BINARY_DIR}/site)

# Exactly the same algorithm sources as the native build (every team member's code).
file(GLOB SEAM_SOURCES CONFIGURE_DEPENDS ${REPO}/src/seam_carving/*.cpp)

add_executable(seam_carving_web ${CMAKE_CURRENT_SOURCE_DIR}/bindings.cpp ${SEAM_SOURCES})
target_include_directories(seam_carving_web PRIVATE ${REPO}/src/seam_carving)
target_compile_options(seam_carving_web PRIVATE -Wall -Wextra -Wpedantic -fexceptions)
target_link_options(seam_carving_web PRIVATE
    -fexceptions
    -lembind
    -sMODULARIZE=1
    -sEXPORT_NAME=createSeamCarving
    -sENVIRONMENT=web,worker
    -sALLOW_MEMORY_GROWTH=1
    -sSINGLE_FILE=1          # the .wasm is embedded in the .js: one file, works on any static host
    -sFILESYSTEM=0)
set_target_properties(seam_carving_web PROPERTIES
    OUTPUT_NAME seam_carving
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY ${SITE_OUT})

# Copy the page (HTML, CSS, JS, sample pictures) next to the compiled module, on every build.
add_custom_target(seam_carving_site ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/site ${SITE_OUT}
    COMMENT "Copying web/site to ${SITE_OUT}")
add_dependencies(seam_carving_web seam_carving_site)
