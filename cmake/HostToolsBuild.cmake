# ==============================================================================
# HostToolsBuild.cmake
# Builds Project Alice's 5 compile-time code generators on the Host (x86_64)
# when cross-compiling the engine for Android (arm64-v8a).
# ==============================================================================

set(ENGINE_ROOT   "${CMAKE_SOURCE_DIR}/src/engine")
set(DCON_INC      "${CMAKE_SOURCE_DIR}/src/dcon")
set(TOOLS_ROOT    "${CMAKE_SOURCE_DIR}/tools")

if(BUILD_HOST_GENERATORS_ONLY)
    message(STATUS "[HostTools] Configuring native x86_64 code generators (all 5)...")

    # 1. DCONGENERATOR
    file(GLOB_RECURSE DCONGEN_SRCS "${TOOLS_ROOT}/DCONGENERATOR/*.cpp")
    add_executable(DCONGENERATOR ${DCONGEN_SRCS})
    target_include_directories(DCONGENERATOR PRIVATE
        "${TOOLS_ROOT}/DCONGENERATOR"
        "${DCON_INC}"
    )

    # 2. ParserGenerator
    add_executable(ParserGenerator "${TOOLS_ROOT}/ParserGenerator/parser_generator.cpp")
    target_include_directories(ParserGenerator PRIVATE
        "${TOOLS_ROOT}/ParserGenerator"
        "${DCON_INC}"
    )

    # 3. DCONINTERFACEGEN (FifInterfaceGenerator)
    add_executable(DCONINTERFACEGEN
        "${TOOLS_ROOT}/DCONINTERFACEGEN/DataContainerGenerator.cpp"
        "${TOOLS_ROOT}/DCONINTERFACEGEN/code_fragments.cpp"
        "${TOOLS_ROOT}/DCONINTERFACEGEN/parsing.cpp"
    )
    target_include_directories(DCONINTERFACEGEN PRIVATE
        "${TOOLS_ROOT}/DCONINTERFACEGEN"
        "${DCON_INC}"
    )

    # 4. DataContainer-Lua
    add_executable(DataContainer-Lua
        "${TOOLS_ROOT}/DataContainer-Lua/src/parser/parsing.cpp"
        "${TOOLS_ROOT}/DataContainer-Lua/src/LuaFFIGenerator.cpp"
    )
    target_compile_definitions(DataContainer-Lua PRIVATE INCREMENTAL=1)
    target_include_directories(DataContainer-Lua PRIVATE "${TOOLS_ROOT}/DataContainer-Lua/src/parser")

    # 5. DataContainer_OOS_Reporter
    add_executable(DataContainer_OOS_Reporter
        "${TOOLS_ROOT}/DataContainer_OOS_Reporter/DataContainer_OOS_Reporter/DataContainer_OOS_Reporter.cpp"
        "${TOOLS_ROOT}/DataContainer_OOS_Reporter/DataContainer_OOS_Reporter/Parsing/parsing.cpp"
    )
    target_include_directories(DataContainer_OOS_Reporter PRIVATE
        "${TOOLS_ROOT}/DataContainer_OOS_Reporter/DataContainer_OOS_Reporter"
        "${TOOLS_ROOT}/DataContainer_OOS_Reporter/DataContainer_OOS_Reporter/Parsing"
    )

    # 6. LuaJIT Host Code Generators (minilua + buildvm_arm64 for W^X-safe interpreter+FFI)
    set(LJ_SRC "${CMAKE_SOURCE_DIR}/upstream/luajit/src")
    set(LJ_HOST_GEN "${CMAKE_BINARY_DIR}/luajit_host")
    file(MAKE_DIRECTORY "${LJ_HOST_GEN}")
    file(WRITE "${LJ_HOST_GEN}/luajit_relver.txt" "1720000000\n")

    add_executable(minilua "${LJ_SRC}/host/minilua.c")
    target_compile_options(minilua PRIVATE -w)

    add_custom_command(
        OUTPUT "${LJ_HOST_GEN}/luajit.h"
               "${LJ_HOST_GEN}/buildvm_arch.h"
        COMMAND $<TARGET_FILE:minilua>
                "${LJ_SRC}/host/genversion.lua"
                "${LJ_SRC}/luajit_rolling.h"
                "${LJ_HOST_GEN}/luajit_relver.txt"
                "${LJ_HOST_GEN}/luajit.h"
        COMMAND $<TARGET_FILE:minilua>
                "${CMAKE_SOURCE_DIR}/upstream/luajit/dynasm/dynasm.lua"
                -D ENDIAN_LE -D P64 -D FFI -D DUALNUM -D FPU -D HFABI -D VER=80
                -o "${LJ_HOST_GEN}/buildvm_arch.h"
                "${LJ_SRC}/vm_arm64.dasc"
        DEPENDS minilua "${LJ_SRC}/vm_arm64.dasc" "${LJ_SRC}/luajit_rolling.h"
        COMMENT "[HostTools] Generating LuaJIT ARM64 VM tables (interpreter + FFI, W^X safe)"
        VERBATIM
    )

    add_executable(buildvm_arm64
        "${LJ_SRC}/host/buildvm.c"
        "${LJ_SRC}/host/buildvm_asm.c"
        "${LJ_SRC}/host/buildvm_peobj.c"
        "${LJ_SRC}/host/buildvm_lib.c"
        "${LJ_SRC}/host/buildvm_fold.c"
        "${LJ_HOST_GEN}/luajit.h"
        "${LJ_HOST_GEN}/buildvm_arch.h"
    )
    target_compile_definitions(buildvm_arm64 PRIVATE
        LUAJIT_TARGET=LUAJIT_ARCH_ARM64
        LUAJIT_OS=LUAJIT_OS_LINUX
        LUAJIT_DISABLE_JIT=1
        LJ_ARCH_HASFPU=1
        LJ_ABI_SOFTFP=0
    )
    target_compile_options(buildvm_arm64 PRIVATE -w)
    target_include_directories(buildvm_arm64 PRIVATE
        "${LJ_HOST_GEN}"
        "${LJ_SRC}"
        "${LJ_SRC}/host"
        "${CMAKE_SOURCE_DIR}/upstream/luajit/dynasm"
    )

    return()
endif()

# ------------------------------------------------------------------------------
# Mode B: Invoked during Android NDK cross-compilation (CMAKE_CROSSCOMPILING=TRUE)
# Runs pre-built host generators from ${HOST_TOOLS_BIN_DIR} to emit all C++ headers
# ------------------------------------------------------------------------------
set(HOST_TOOLS_BIN_DIR "${CMAKE_SOURCE_DIR}/build-host" CACHE PATH "Directory containing host x86_64 generator binaries")
set(GEN_OUT_DIR "${CMAKE_BINARY_DIR}/generated" CACHE PATH "Output directory for generated C++ headers")
file(MAKE_DIRECTORY
    "${GEN_OUT_DIR}/gamestate"
    "${GEN_OUT_DIR}/parsing"
    "${GEN_OUT_DIR}/text"
    "${GEN_OUT_DIR}/lua_engine"
)

if(CMAKE_HOST_WIN32)
    set(HOST_EXE_EXT ".exe")
else()
    set(HOST_EXE_EXT "")
endif()

set(DCONGEN_BIN   "${HOST_TOOLS_BIN_DIR}/DCONGENERATOR${HOST_EXE_EXT}")
set(DCONIFACE_BIN "${HOST_TOOLS_BIN_DIR}/DCONINTERFACEGEN${HOST_EXE_EXT}")
set(DCONLUA_BIN   "${HOST_TOOLS_BIN_DIR}/DataContainer-Lua${HOST_EXE_EXT}")
set(DCONOOS_BIN   "${HOST_TOOLS_BIN_DIR}/DataContainer_OOS_Reporter${HOST_EXE_EXT}")
set(PARSERGEN_BIN "${HOST_TOOLS_BIN_DIR}/ParserGenerator${HOST_EXE_EXT}")

set(DCON_SCHEMA_SRC "${ENGINE_ROOT}/gamestate/dcon_generated.txt")
set(DCON_SCHEMA_DST "${GEN_OUT_DIR}/gamestate/dcon_generated.txt")

add_custom_command(
    OUTPUT "${GEN_OUT_DIR}/gamestate/dcon_generated.hpp"
           "${GEN_OUT_DIR}/gamestate/dcon_generated_ids.hpp"
           "${GEN_OUT_DIR}/gamestate/fif_dcon_generated.hpp"
           "${GEN_OUT_DIR}/gamestate/lua_dcon_generated.hpp"
           "${GEN_OUT_DIR}/gamestate/lua_dcon_generated.cpp"
           "${GEN_OUT_DIR}/gamestate/dcon_oos_reporter_generated.hpp"
           "${GEN_OUT_DIR}/gamestate/dcon_oos_reporter_generated.cpp"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DCON_SCHEMA_SRC}" "${DCON_SCHEMA_DST}"
    COMMAND "${DCONGEN_BIN}"   "${DCON_SCHEMA_DST}"
    COMMAND "${DCONIFACE_BIN}" "${DCON_SCHEMA_DST}"
    COMMAND "${DCONLUA_BIN}"   alice alice_state_ptr->world. "${DCON_SCHEMA_DST}"
            "${GEN_OUT_DIR}/gamestate/lua_dcon_generated.cpp"
            "${GEN_OUT_DIR}/gamestate/lua_dcon_generated.hpp"
            "${GEN_OUT_DIR}/lua_engine"
    COMMAND "${DCONOOS_BIN}"   alice "${DCON_SCHEMA_DST}"
            dcon_oos_reporter_generated.cpp
            dcon_oos_reporter_generated.hpp
            "${GEN_OUT_DIR}/gamestate/"
    WORKING_DIRECTORY "${GEN_OUT_DIR}/gamestate"
    DEPENDS "${DCON_SCHEMA_SRC}"
    COMMENT "[CodeGen] Running all 4 DataContainer host generators"
    VERBATIM
)

set(PARSER_SCHEMAS
    "parsing/parser_defs"
    "parsing/tutorial_parser_defs"
    "parsing/gui_parser_defs"
    "parsing/save_parser_defs"
    "parsing/news_parser_defs"
    "parsing/effect_parser_defs"
    "parsing/trigger_parser_defs"
    "text/font_defs"
    "text/bmfont_defs"
)

set(GENERATED_PARSER_HEADERS "")
foreach(SCHEMA ${PARSER_SCHEMAS})
    set(SRC_TXT "${ENGINE_ROOT}/${SCHEMA}.txt")
    set(DST_TXT "${GEN_OUT_DIR}/${SCHEMA}.txt")
    set(OUT_HPP "${GEN_OUT_DIR}/${SCHEMA}_generated.hpp")
    list(APPEND GENERATED_PARSER_HEADERS "${OUT_HPP}")

    add_custom_command(
        OUTPUT "${OUT_HPP}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_TXT}" "${DST_TXT}"
        COMMAND "${PARSERGEN_BIN}" "${DST_TXT}"
        DEPENDS "${SRC_TXT}"
        COMMENT "[CodeGen] Generating ${SCHEMA}_generated.hpp"
        VERBATIM
    )
endforeach()

add_custom_target(Vic2MobileCodeGen DEPENDS
    "${GEN_OUT_DIR}/gamestate/dcon_generated.hpp"
    "${GEN_OUT_DIR}/gamestate/fif_dcon_generated.hpp"
    "${GEN_OUT_DIR}/gamestate/lua_dcon_generated.hpp"
    "${GEN_OUT_DIR}/gamestate/dcon_oos_reporter_generated.hpp"
    ${GENERATED_PARSER_HEADERS}
)
