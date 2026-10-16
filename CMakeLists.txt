cmake_minimum_required(VERSION 3.18)
project(NullFrequency VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_GAME "Build the graphical game" ON)
option(NF_STATIC_RUNTIME "Statically link compiler runtimes where supported" OFF)
if(NF_STATIC_RUNTIME AND MSVC)
    # Keep older dependency projects on the same runtime-selection policy.
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
include(CTest)

add_library(nf_core STATIC src/core.cpp)
target_include_directories(nf_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(MSVC)
    target_compile_options(nf_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(nf_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(null-frequency-core-tests tests/core_tests.cpp)
    target_link_libraries(null-frequency-core-tests PRIVATE nf_core)
    if(MSVC)
        target_compile_options(null-frequency-core-tests PRIVATE /utf-8)
    endif()
    add_test(NAME core COMMAND null-frequency-core-tests)
endif()

if(BUILD_GAME)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(raylib
        URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5
        URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    )
    FetchContent_MakeAvailable(raylib)

    add_executable(null-frequency src/main.cpp src/audio.cpp)
    target_link_libraries(null-frequency PRIVATE nf_core raylib)
    target_include_directories(null-frequency SYSTEM PRIVATE "${raylib_SOURCE_DIR}/src")
    if(MSVC)
        target_compile_options(null-frequency PRIVATE /W4 /utf-8)
    else()
        target_compile_options(null-frequency PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(NF_STATIC_RUNTIME AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(null-frequency PRIVATE -static-libstdc++ -static-libgcc)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # raylib's OpenGL discovery can also list GLU, which this game does not use.
        target_link_options(null-frequency PRIVATE -Wl,--as-needed)
    endif()
    add_custom_command(TARGET null-frequency POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:null-frequency>/assets"
        COMMENT "Copying game assets"
    )
    # An explicit game-target build still gets its initial assets above. The
    # default build also refreshes Blender exports and textures when no C++
    # source changed, so an up-to-date executable cannot keep stale assets.
    add_custom_target(null-frequency-assets ALL
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:null-frequency>/assets"
        DEPENDS null-frequency
        COMMENT "Refreshing game assets"
    )
    install(TARGETS null-frequency RUNTIME DESTINATION . COMPONENT Game)
    install(DIRECTORY assets DESTINATION . COMPONENT Game)
    install(FILES README.md LICENSE THIRD_PARTY.md DESTINATION . COMPONENT Game)
    install(FILES "${raylib_SOURCE_DIR}/LICENSE" DESTINATION licenses RENAME raylib-LICENSE.txt COMPONENT Game)
    install(FILES "${raylib_SOURCE_DIR}/src/external/glfw/LICENSE.md" DESTINATION licenses RENAME GLFW-LICENSE.txt COMPONENT Game)
    install(FILES docs/screenshot.png DESTINATION docs COMPONENT Game OPTIONAL)
    if(UNIX)
        install(PROGRAMS run.sh DESTINATION . COMPONENT Game)
    endif()
endif()
