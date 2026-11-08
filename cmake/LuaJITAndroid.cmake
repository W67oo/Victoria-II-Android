# ==============================================================================
# LuaJITAndroid.cmake
# Cross-compiles LuaJIT v2.1 for Android arm64-v8a in W^X-safe Interpreter + FFI
# mode (-DLUAJIT_DISABLE_JIT) using pre-built host buildvm_arm64.
# ==============================================================================

enable_language(ASM)

set(LJ_SRC     "${CMAKE_SOURCE_DIR}/upstream/luajit/src")
set(LJ_GEN_DIR "${CMAKE_BINARY_DIR}/generated/luajit")
file(MAKE_DIRECTORY "${LJ_GEN_DIR}")

set(MINILUA_BIN "${HOST_TOOLS_BIN_DIR}/minilua${HOST_EXE_EXT}")
set(BUILDVM_BIN "${HOST_TOOLS_BIN_DIR}/buildvm_arm64${HOST_EXE_EXT}")

file(WRITE "${LJ_GEN_DIR}/luajit_relver.txt" "1720000000\n")

set(LJ_LIB_SRCS
    "${LJ_SRC}/lib_base.c"
    "${LJ_SRC}/lib_math.c"
    "${LJ_SRC}/lib_bit.c"
    "${LJ_SRC}/lib_string.c"
    "${LJ_SRC}/lib_table.c"
    "${LJ_SRC}/lib_io.c"
    "${LJ_SRC}/lib_os.c"
    "${LJ_SRC}/lib_package.c"
    "${LJ_SRC}/lib_debug.c"
    "${LJ_SRC}/lib_jit.c"
    "${LJ_SRC}/lib_ffi.c"
    "${LJ_SRC}/lib_buffer.c"
)

add_custom_command(
    OUTPUT "${LJ_GEN_DIR}/luajit.h"
           "${LJ_GEN_DIR}/lj_vm.S"
           "${LJ_GEN_DIR}/lj_bcdef.h"
           "${LJ_GEN_DIR}/lj_ffdef.h"
           "${LJ_GEN_DIR}/lj_libdef.h"
           "${LJ_GEN_DIR}/lj_recdef.h"
           "${LJ_GEN_DIR}/lj_folddef.h"
    COMMAND "${MINILUA_BIN}" "${LJ_SRC}/host/genversion.lua"
            "${LJ_SRC}/luajit_rolling.h"
            "${LJ_GEN_DIR}/luajit_relver.txt"
            "${LJ_GEN_DIR}/luajit.h"
    COMMAND "${BUILDVM_BIN}" -m elfasm  -o "${LJ_GEN_DIR}/lj_vm.S"
    COMMAND "${BUILDVM_BIN}" -m bcdef   -o "${LJ_GEN_DIR}/lj_bcdef.h"   ${LJ_LIB_SRCS}
    COMMAND "${BUILDVM_BIN}" -m ffdef   -o "${LJ_GEN_DIR}/lj_ffdef.h"   ${LJ_LIB_SRCS}
    COMMAND "${BUILDVM_BIN}" -m libdef  -o "${LJ_GEN_DIR}/lj_libdef.h"  ${LJ_LIB_SRCS}
    COMMAND "${BUILDVM_BIN}" -m recdef  -o "${LJ_GEN_DIR}/lj_recdef.h"  ${LJ_LIB_SRCS}
    COMMAND "${BUILDVM_BIN}" -m folddef -o "${LJ_GEN_DIR}/lj_folddef.h" "${LJ_SRC}/lj_opt_fold.c"
    DEPENDS ${LJ_LIB_SRCS} "${LJ_SRC}/lj_opt_fold.c"
    COMMENT "[LuaJIT] Generating ARM64 ELF interpreter VM (lj_vm.S) and FFI headers"
    VERBATIM
)

add_library(luajit_arm64 STATIC
    "${LJ_GEN_DIR}/lj_vm.S"
    "${LJ_SRC}/ljamalg.c"
    "${LJ_GEN_DIR}/luajit.h"
    "${LJ_GEN_DIR}/lj_bcdef.h"
    "${LJ_GEN_DIR}/lj_ffdef.h"
    "${LJ_GEN_DIR}/lj_libdef.h"
    "${LJ_GEN_DIR}/lj_recdef.h"
    "${LJ_GEN_DIR}/lj_folddef.h"
)

target_compile_definitions(luajit_arm64 PRIVATE
    LUAJIT_TARGET=LUAJIT_ARCH_ARM64
    LUAJIT_OS=LUAJIT_OS_LINUX
    LUAJIT_DISABLE_JIT=1
    LJ_ARCH_HASFPU=1
    LJ_ABI_SOFTFP=0
    _FILE_OFFSET_BITS=64
    _LARGEFILE_SOURCE
)

target_compile_options(luajit_arm64 PRIVATE
    -O2
    -fPIC
    -w
    -fno-stack-protector
)

target_include_directories(luajit_arm64 PUBLIC
    "${LJ_GEN_DIR}"
    "${LJ_SRC}"
)
