cmake_minimum_required(VERSION 3.16)
project(GlitchCraftGameCubeEdition LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GLITCHCRAFT_BUILD_TESTS "Build the game engine tests" ON)

set(GLITCHCRAFT_KERNEL_SOURCE GameEngine/src/voxel_kernels.c)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc|powerpc64|ppc64|powerpc64le|ppc64le)$")
    enable_language(ASM)
    set(GLITCHCRAFT_KERNEL_SOURCE GameEngine/platform/powerpc/voxel_kernels.S)
endif()

add_library(glitchcraft_engine
    src/engine.cpp
    src/c_api.cpp
    src/realms.cpp
    src/portable_math.c
    GameEngine/src/renderer.cpp
    GameEngine/src/window.cpp
    ${GLITCHCRAFT_KERNEL_SOURCE}
)
target_include_directories(glitchcraft_engine
    PUBLIC include GameEngine/include
    PRIVATE src
)
target_compile_options(glitchcraft_engine PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion>
)

find_package(X11 QUIET)
if(X11_FOUND)
    target_compile_definitions(glitchcraft_engine PRIVATE GLITCHCRAFT_HAS_X11)
    target_include_directories(glitchcraft_engine PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(glitchcraft_engine PUBLIC ${X11_LIBRARIES})
endif()

add_executable(glitchcraft src/main.cpp src/console.cpp)
target_include_directories(glitchcraft PRIVATE include src)
target_link_libraries(glitchcraft PRIVATE glitchcraft_engine)
if(WIN32)
    target_link_libraries(glitchcraft_engine PUBLIC ws2_32)
endif()
target_compile_options(glitchcraft PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion>
)

if(GLITCHCRAFT_BUILD_TESTS)
    enable_testing()
    find_package(Threads REQUIRED)
    add_executable(glitchcraft_tests tests/test_engine.cpp)
    target_include_directories(glitchcraft_tests PRIVATE src)
    target_link_libraries(glitchcraft_tests PRIVATE glitchcraft_engine
        Threads::Threads)
    add_test(NAME glitchcraft_engine_tests COMMAND glitchcraft_tests)
endif()
