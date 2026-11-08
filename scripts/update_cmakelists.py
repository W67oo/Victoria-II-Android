import re

with open("CMakeLists.txt", "r", encoding="utf-8") as f:
    content = f.read()

# 1. Wrap SDL2 in if(ANDROID)
old_sdl2 = """# 2. SDL2 (Android windowing, OpenGL ES surface, multi-touch, soft keyboard -> libSDL2.so)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)   # Audio handled directly via miniaudio AAudio
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/upstream/SDL2" "${CMAKE_BINARY_DIR}/SDL2" EXCLUDE_FROM_ALL)"""

new_sdl2 = """# 2. SDL2 (Android windowing, OpenGL ES surface, multi-touch, soft keyboard -> libSDL2.so)
if(ANDROID)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_AUDIO OFF CACHE BOOL "" FORCE)   # Audio handled directly via miniaudio AAudio
    set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/upstream/SDL2" "${CMAKE_BINARY_DIR}/SDL2" EXCLUDE_FROM_ALL)
endif()"""

content = content.replace(old_sdl2, new_sdl2)

# 2. Wrap LuaJITAndroid.cmake in if(ANDROID)
old_lj = 'include("${CMAKE_SOURCE_DIR}/cmake/LuaJITAndroid.cmake")'
new_lj = 'if(ANDROID)\n    include("${CMAKE_SOURCE_DIR}/cmake/LuaJITAndroid.cmake")\nendif()'
content = content.replace(old_lj, new_lj)

# 3. Target definitions: replace from add_library(vic2mobile SHARED ... to the end of file
target_def_pos = content.find("add_library(vic2mobile SHARED")
assert target_def_pos != -1, "Target definition not found!"

targets_section = """# ==============================================================================
# Target Definitions (Android: vic2mobile.so | Host: vic2_headless_sim.exe)
# ==============================================================================
set(ALICE_ENGINE_INCLUDES
    "${GEN_OUT_DIR}/gamestate"
    "${GEN_OUT_DIR}/parsing"
    "${GEN_OUT_DIR}/text"
    "${DCON_INC}"
    "${ALICE_SRC}"
    "${ALICE_SRC}/ai"
    "${ALICE_SRC}/common_types"
    "${ALICE_SRC}/culture"
    "${ALICE_SRC}/economy"
    "${ALICE_SRC}/filesystem"
    "${ALICE_SRC}/gamerule"
    "${ALICE_SRC}/gamestate"
    "${ALICE_SRC}/graphics"
    "${ALICE_SRC}/gui"
    "${ALICE_SRC}/gui/province_tiles"
    "${ALICE_SRC}/gui/widgets"
    "${ALICE_SRC}/gui/topbar_subwindows"
    "${ALICE_SRC}/gui/topbar_subwindows/diplomacy_subwindows"
    "${ALICE_SRC}/gui/topbar_subwindows/production_subwindows"
    "${ALICE_SRC}/gui/topbar_subwindows/politics_subwindows"
    "${ALICE_SRC}/gui/topbar_subwindows/military_subwindows"
    "${ALICE_SRC}/logistics"
    "${ALICE_SRC}/lunasvg"
    "${ALICE_SRC}/map"
    "${ALICE_SRC}/military"
    "${ALICE_SRC}/nations"
    "${ALICE_SRC}/network"
    "${ALICE_SRC}/network/webapi"
    "${ALICE_SRC}/parsing"
    "${ALICE_SRC}/provinces"
    "${ALICE_SRC}/scripting"
    "${ALICE_SRC}/scripting/luajit"
    "${ALICE_SRC}/sound"
    "${ALICE_SRC}/text"
    "${ALICE_SRC}/window"
    "${ALICE_SRC}/zstd"
    "${CMAKE_SOURCE_DIR}/src/engine/dependencies/miniaudio"
    "${CMAKE_SOURCE_DIR}/src/engine/dependencies/stb"
    "${CMAKE_SOURCE_DIR}/upstream/simde"
    "${CMAKE_SOURCE_DIR}/upstream/glm"
    "${CMAKE_SOURCE_DIR}/upstream/fmt/include"
    "${CMAKE_SOURCE_DIR}/upstream/freetype/include"
    "${CMAKE_SOURCE_DIR}/upstream/harfbuzz/src"
)

set(COMMON_PCH_HEADERS
    "$<$<COMPILE_LANGUAGE:CXX>:<stddef.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<stdint.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<assert.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<stdarg.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<stdlib.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<math.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<signal.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<ctype.h>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<vector>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<queue>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<array>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<optional>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<memory>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<variant>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<string>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<string_view>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<charconv>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<algorithm>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<iterator>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<functional>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<atomic>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<chrono>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<tuple>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<type_traits>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<new>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<limits>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<utility>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<any>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<shared_mutex>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<cstdlib>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<cstddef>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<cstdint>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<cassert>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<cstdarg>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<cmath>>"
    "$<$<COMPILE_LANGUAGE:CXX>:\"simple_fs.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"constants.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"unordered_dense.h\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"dcon_generated.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"dcon_generated_ids.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"container_types.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"parsers_declarations.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"cyto_any.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"date_interface.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"stb_image.h\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"glm/glm.hpp\">"
    "$<$<COMPILE_LANGUAGE:CXX>:\"miniaudio.h\">"
)

if(ANDROID)
    add_library(vic2mobile SHARED
        "${CMAKE_SOURCE_DIR}/src/platform_android/platform_android_unity.cpp"
        ${ALICE_PLUTOVG_C_SOURCES}
        ${ALICE_ENGINE_CPP_SOURCES}
    )

    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/src/platform_android/platform_android_unity.cpp"
        ${ALICE_PLUTOVG_C_SOURCES}
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )

    add_dependencies(vic2mobile Vic2MobileCodeGen)

    target_compile_definitions(vic2mobile PRIVATE
        INCREMENTAL=1
        ALICE_NO_ENTRY_POINT=1
        GLM_ENABLE_EXPERIMENTAL=1
        DO_NOT_USE_LLVM=1          # Portable Fif bytecode interpreter on ARM64 (SELinux W^X safe)
        PREFER_ONE_TBB=1           # Use oneTBB task scheduler capped to big cores
        DCON_GLOBAL_BACKING_MULTIPLIER=1
        SIMDE_ENABLE_NATIVE_ALIASES=1
        VIC2_ANDROID_MOBILE=1
        PROJECT_ROOT="${CMAKE_SOURCE_DIR}/src/engine"
        $<$<BOOL:${VIC2_DISABLE_MULTIPLAYER}>:VIC2_NO_MULTIPLAYER=1>
    )

    target_compile_options(vic2mobile PRIVATE
        -Wall
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unused-private-field
        -Wno-unused-function
        -Wno-unused-lambda-capture
        -Wno-switch
        -Wno-invalid-offsetof
        -Wno-dangling-else
        -Wno-microsoft-cast
        -fstrict-aliasing
        -fno-fast-math
        -ffp-model=precise
        -ffp-contract=off
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
    )

    target_link_options(vic2mobile PRIVATE
        -Wl,--export-dynamic
    )

    # Force-include ARM64 SIMD compatibility shim on AArch64 before DataContainer ve.hpp
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
        target_compile_options(vic2mobile PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include \"${CMAKE_SOURCE_DIR}/src/platform_android/simd_arm64_compat.hpp\">"
        )
    endif()

    target_include_directories(vic2mobile PRIVATE
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},aarch64>:${CMAKE_SOURCE_DIR}/src/platform_android/arm64_shim>
        "${CMAKE_SOURCE_DIR}/src/platform_android"
        "${CMAKE_SOURCE_DIR}/upstream/SDL2/include"
        ${ALICE_ENGINE_INCLUDES}
    )

    target_precompile_headers(vic2mobile PRIVATE ${COMMON_PCH_HEADERS})

    target_link_libraries(vic2mobile PRIVATE
        tbb
        SDL2::SDL2
        freetype
        harfbuzz
        fmt::fmt
        luajit_arm64
        android
        EGL
        GLESv3
        dl
        log
        m
        z
    )

else() # Host Windows build -> vic2_headless_sim executable

    add_executable(vic2_headless_sim
        "${CMAKE_SOURCE_DIR}/tools/headless_sim/main.cpp"
        "${CMAKE_SOURCE_DIR}/tools/headless_sim/platform_sim.cpp"
        ${ALICE_PLUTOVG_C_SOURCES}
        ${ALICE_ENGINE_CPP_SOURCES}
    )

    set_source_files_properties(
        "${CMAKE_SOURCE_DIR}/tools/headless_sim/platform_sim.cpp"
        ${ALICE_PLUTOVG_C_SOURCES}
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )

    add_dependencies(vic2_headless_sim Vic2MobileCodeGen)

    target_compile_definitions(vic2_headless_sim PRIVATE
        INCREMENTAL=1
        ALICE_NO_ENTRY_POINT=1
        GLM_ENABLE_EXPERIMENTAL=1
        DO_NOT_USE_LLVM=1
        PREFER_ONE_TBB=1
        DCON_GLOBAL_BACKING_MULTIPLIER=1
        SIMDE_ENABLE_NATIVE_ALIASES=1
        VIC2_NO_MULTIPLAYER=1
        PROJECT_ROOT="${CMAKE_SOURCE_DIR}/src/engine"
        NOMINMAX=1
        WIN32_LEAN_AND_MEAN=1
    )

    target_compile_options(vic2_headless_sim PRIVATE
        -Wall
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unused-private-field
        -Wno-unused-function
        -Wno-unused-lambda-capture
        -Wno-switch
        -Wno-invalid-offsetof
        -Wno-dangling-else
        -Wno-microsoft-cast
        -fstrict-aliasing
        -fno-fast-math
        -ffp-model=precise
        -ffp-contract=off
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
    )

    target_include_directories(vic2_headless_sim PRIVATE
        "${CMAKE_SOURCE_DIR}/tools/headless_sim/include"
        "${CMAKE_SOURCE_DIR}/tools/headless_sim/include/unicode"
        ${ALICE_ENGINE_INCLUDES}
    )

    target_precompile_headers(vic2_headless_sim PRIVATE ${COMMON_PCH_HEADERS})

    target_link_libraries(vic2_headless_sim PRIVATE
        tbb
        freetype
        harfbuzz
        fmt::fmt
        "${CMAKE_SOURCE_DIR}/upstream/Project-Alice/libs/luajit.lib"
        ws2_32
        ole32
        shell32
        user32
        kernel32
        advapi32
    )

endif()
"""

new_content = content[:target_def_pos] + targets_section
with open("CMakeLists.txt", "w", encoding="utf-8") as f:
    f.write(new_content)
print("Updated CMakeLists.txt successfully.")
