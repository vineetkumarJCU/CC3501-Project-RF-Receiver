cmake_minimum_required(VERSION 3.20)

project(rp2040_radio_gui VERSION 1.0.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project is configured for Windows and SDL2.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep generated executables in one predictable directory for all generators.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/bin")
endforeach()

# Build the vendored SDL2 source as a static library.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# Build all LVGL core sources and the repository's demo sources.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
add_subdirectory(lvgl)

# The built-in LVGL SDL driver references SDL directly.
target_link_libraries(lvgl PUBLIC SDL2::SDL2-static)

add_library(radio_gui STATIC
    src/gui/radio_gui.c
    src/gui/radio_gui_radio.c
    src/gui/radio_gui_waveform.c
    src/gui/radio_gui_hardware.c
    src/gui/radio_gui_gps.c
)
target_include_directories(radio_gui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(radio_gui PUBLIC lvgl::lvgl)

add_executable(rp2040_radio_gui_demo
    src/main.c
    src/demo/radio_gui_demo_backend.c
)
target_include_directories(rp2040_radio_gui_demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(rp2040_radio_gui_demo PRIVATE radio_gui)

if(MINGW)
    # Produce a self-contained executable without MinGW runtime DLLs.
    target_link_options(rp2040_radio_gui_demo PRIVATE -static -static-libgcc -static-libstdc++)
endif()
