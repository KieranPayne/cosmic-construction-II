cmake_minimum_required(VERSION 3.28)
project(CCII LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(BUILD_SHARED_LIBS OFF)
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# SET(GCC_COVERAGE_COMPILE_FLAGS "-O3")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
add_executable(CCII ${SOURCES})


target_compile_features(CCII PRIVATE cxx_std_20)
target_link_libraries(CCII PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network)
target_link_options(CCII PRIVATE -static-libgcc -static-libstdc++ -static)
add_custom_target(copy_content ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/content
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/content
    DEPENDS ${SOURCES}
)


add_definitions(-w)
add_dependencies(CCII copy_content)
