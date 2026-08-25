cmake_minimum_required(VERSION 3.28)
project(Civitron LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
add_executable(Civitron ${SOURCES})


target_compile_features(Civitron PRIVATE cxx_std_20)
target_link_libraries(Civitron PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network)
add_custom_target(copy_content ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/content
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/content
    DEPENDS ${SOURCES}
)


add_definitions(-w)
add_dependencies(Civitron copy_content)
