cmake_minimum_required(VERSION 3.27)
project(gramarye-libcore C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Projects that reference this often compile to WebAssembly via Emscripten
# so we provide a toggle to facilitate that.

# WebAssembly toggle (must be before fetching raylib)
option(BUILD_WEB "Build with Emscripten (WebAssembly)" OFF)

# When building for web (via -DBUILD_WEB=ON or emcmake), set platform and output suffix
if(EMSCRIPTEN OR BUILD_WEB)
    # Ensure raylib configures for Web before it's fetched/built
    set(PLATFORM Web CACHE STRING "" FORCE)
    # Make final output an .html launcher
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# Option to build as shared or static (default static)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(GRAMARYE_THREADING "Enable synchronization primitives for multithreading" ON)
set(GRAMARYE_ATOMICS_BACKEND "auto" CACHE STRING "Atomic backend: auto, c11, builtin, msvc, single")
set_property(CACHE GRAMARYE_ATOMICS_BACKEND PROPERTY STRINGS auto c11 builtin msvc single)

# Source files
file(GLOB SRC_FILES
    "src/*.c"
    "src/hash/*.c"
)

# Filter out libc replacements - use system libc
list(FILTER SRC_FILES EXCLUDE REGEX ".*/strerror\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/strncmp\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/memcmp\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/memmove\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/thread\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/thread-nt\\.c$")

message(STATUS "SRC_FILES: ${SRC_FILES}
                THREADING: ${GRAMARYE_THREADING}
                ATOMICS_BACKEND: ${GRAMARYE_ATOMICS_BACKEND}")

# Create library
add_library(gramarye-libcore 
    ${SRC_FILES}
)

if(GRAMARYE_THREADING)
    if(NOT WIN32)
        find_package(Threads REQUIRED)
        target_link_libraries(gramarye-libcore PUBLIC Threads::Threads)
    endif()
else()
    target_compile_definitions(gramarye-libcore PUBLIC GRAMARYE_SYNC_SINGLE_THREADED GRAMARYE_ATOMICS_SINGLE)
endif()

if(NOT GRAMARYE_ATOMICS_BACKEND STREQUAL "auto")
    if(GRAMARYE_ATOMICS_BACKEND STREQUAL "single")
        target_compile_definitions(gramarye-libcore PUBLIC GRAMARYE_ATOMICS_SINGLE)
    elseif(GRAMARYE_ATOMICS_BACKEND STREQUAL "builtin")
        target_compile_definitions(gramarye-libcore PUBLIC GRAMARYE_ATOMICS_BUILTIN)
    elseif(GRAMARYE_ATOMICS_BACKEND STREQUAL "msvc")
        target_compile_definitions(gramarye-libcore PUBLIC GRAMARYE_ATOMICS_MSVC)
    elseif(GRAMARYE_ATOMICS_BACKEND STREQUAL "c11")
        target_compile_definitions(gramarye-libcore PUBLIC GRAMARYE_ATOMICS_C11)
    endif()
elseif(NOT CMAKE_C_STANDARD LESS 11)
    target_compile_definitions(gramarye-libcore PUBLIC GRAMARYE_ATOMICS_C11)
else()
    target_compile_definitions(gramarye-libcore PUBLIC GRAMARYE_ATOMICS_BUILTIN)
endif()

# Add include directory for hash headers
target_include_directories(gramarye-libcore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/hash>
    $<INSTALL_INTERFACE:include>
)

# Export for CMake
include(GNUInstallDirs)
install(TARGETS gramarye-libcore
    EXPORT coreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT coreTargets
    FILE coreTargets.cmake
    NAMESPACE gramarye-libcore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/core
)

# For FetchContent compatibility - create an interface target
# This allows the library to work both when installed and when used via FetchContent
if(NOT TARGET gramarye-libcore::gramarye-libcore)
    add_library(gramarye-libcore::gramarye-libcore ALIAS gramarye-libcore)
endif()