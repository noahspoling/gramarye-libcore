cmake_minimum_required(VERSION 3.27)
project(xmos-libcore C)
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

# Source files
file(GLOB SRC_FILES
    "src/*.c"
    "src/hash/*.c"
)

# Platform-specific files
if(WIN32)
    file(GLOB PLATFORM_SOURCES "src/core/platform/win/*.c")
else()
    file(GLOB PLATFORM_SOURCES "src/core/platform/unix/*.c")
endif()

# Filter out libc replacements - use system libc
list(FILTER SRC_FILES EXCLUDE REGEX ".*/strerror\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/strncmp\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/memcmp\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/memmove\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/thread\\.c$")
list(FILTER SRC_FILES EXCLUDE REGEX ".*/thread-nt\\.c$")

message(STATUS "SRC_FILES: ${SRC_FILES}
                PLATFORM_FILES: ${PLATFORM_FILES}")

# Create library
add_library(xmos-libcore 
    ${SRC_FILES}
    ${PLATFORM_SOURCES}
)

# Add include directory for hash headers
target_include_directories(xmos-libcore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/hash>
    $<INSTALL_INTERFACE:include>
)

# Export for CMake
include(GNUInstallDirs)
install(TARGETS xmos-libcore
    EXPORT coreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT coreTargets
    FILE coreTargets.cmake
    NAMESPACE xmos-libcore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/core
)

# For FetchContent compatibility - create an interface target
# This allows the library to work both when installed and when used via FetchContent
if(NOT TARGET xmos-libcore::xmos-libcore)
    add_library(xmos-libcore::xmos-libcore ALIAS xmos-libcore)
endif()