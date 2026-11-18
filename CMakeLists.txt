cmake_minimum_required(VERSION 3.28)
project(CCII LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(BUILD_SHARED_LIBS OFF)

# Automatically use sccache or ccache if installed on your machine
find_program(CCACHE_PROGRAM NAMES sccache ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(SFML
    URL https://github.com/SFML/SFML/archive/refs/tags/3.0.1.tar.gz
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

# Only glob .cpp files; re-run CMake manually when adding new source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(CCII ${SOURCES})

target_compile_features(CCII PRIVATE cxx_std_20)
target_compile_options(CCII PRIVATE -w)

# Ensure ImGui math operators are enabled before any header (including PCH) is parsed
target_compile_definitions(CCII PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# Precompile your shared headers for game source files
target_precompile_headers(CCII PRIVATE
    src/PCH.hpp
    src/json.hpp
)

# Exclude third-party ImGui .cpp files from using the precompiled header
file(GLOB IMGUI_SOURCES "src/imgui/*.cpp")
set_source_files_properties(${IMGUI_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

target_link_libraries(CCII PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
)

# Use lld for faster linking if your toolchain supports it
include(CheckLinkerFlag)
check_linker_flag(CXX "-fuse-ld=lld" HAS_LLD)
if(HAS_LLD)
    target_link_options(CCII PRIVATE "-fuse-ld=lld")
endif()

# Only force full static linking in Release builds to keep incremental links fast
target_link_options(CCII PRIVATE
    -static-libgcc
    -static-libstdc++
    $<$<CONFIG:Release>:-static>
)

# Only copy modified assets instead of overwriting the whole content directory every build
add_custom_target(copy_content ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_SOURCE_DIR}/content
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/content
)
add_dependencies(CCII copy_content)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)