cmake_minimum_required(VERSION 3.22)

project(Vic2Mobile LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_HOST_GENERATORS_ONLY "Build only x86_64 host code generators" OFF)
option(VIC2_DISABLE_MULTIPLAYER   "Strip desktop multiplayer socket code for mobile singleplayer" ON)

if(BUILD_HOST_GENERATORS_ONLY)
    include("${CMAKE_SOURCE_DIR}/cmake/HostToolsBuild.cmake")
    return()
endif()

# ==============================================================================
# Android NDK Target Build (arm64-v8a -> libvic2mobile.so)
# ==============================================================================
if(ANDROID)
    if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
        message(FATAL_ERROR "Vic2Mobile targets 64-bit arm64-v8a exclusively (requires >4GB address space & ARMv8 NEON).")
    endif()
    add_link_options("-Wl,-z,max-page-size=16384")
endif()

# 1. oneTBB (ARM64 shared task scheduler -> libtbb.so)
set(BUILD_SHARED_LIBS ON)
set(TBB_TEST OFF CACHE BOOL "" FORCE)
set(TBB_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TBB_BENCH OFF CACHE BOOL "" FORCE)
set(TBB_STRICT OFF CACHE BOOL "" FORCE)
set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)
set(TBB_DISABLE_HWLOC_AUTOMATIC_SEARCH ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/upstream/oneTBB" "${CMAKE_BINARY_DIR}/oneTBB" EXCLUDE_FROM_ALL)
set_target_properties(tbb PROPERTIES DEBUG_POSTFIX "")

# 2. SDL2 (Android windowing, OpenGL ES surface, multi-touch, soft keyboard -> libSDL2.so)
if(ANDROID)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_AUDIO OFF CACHE BOOL "" FORCE)   # Audio handled directly via miniaudio AAudio
    set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/upstream/SDL2" "${CMAKE_BINARY_DIR}/SDL2" EXCLUDE_FROM_ALL)
endif()

# 3. FreeType 2 (static, SDF font rasterizer)
set(BUILD_SHARED_LIBS OFF)
set(FT_DISABLE_ZLIB OFF CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/upstream/freetype" "${CMAKE_BINARY_DIR}/freetype" EXCLUDE_FROM_ALL)

# 4. HarfBuzz (static, OpenType & CJK text shaping with FreeType interop)
set(HB_HAVE_FREETYPE ON CACHE BOOL "" FORCE)
set(HB_HAVE_ICU OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GLIB OFF CACHE BOOL "" FORCE)
set(HB_HAVE_CAIRO OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
set(HB_BUILD_RASTER OFF CACHE BOOL "" FORCE)
set(HB_BUILD_VECTOR OFF CACHE BOOL "" FORCE)
set(HB_BUILD_GPU OFF CACHE BOOL "" FORCE)
set(HB_BUILD_GPU_DEMO "OFF" CACHE STRING "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/upstream/harfbuzz" "${CMAKE_BINARY_DIR}/harfbuzz" EXCLUDE_FROM_ALL)

# 5. fmt (static formatting library)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/upstream/fmt" "${CMAKE_BINARY_DIR}/fmt" EXCLUDE_FROM_ALL)
unset(BUILD_SHARED_LIBS)

# 6. Host Code Generators (DCON, Parsers, Lua FFI, OOS) + LuaJIT ARM64 W^X Interpreter
include("${CMAKE_SOURCE_DIR}/cmake/HostToolsBuild.cmake")
if(ANDROID)
    include("${CMAKE_SOURCE_DIR}/cmake/LuaJITAndroid.cmake")
endif()

set(ALICE_SRC "${CMAKE_SOURCE_DIR}/src/engine")
set(DCON_INC  "${CMAKE_SOURCE_DIR}/src/dcon")

# Complete deduplicated Project Alice C/C++ engine sources (excluding platform_specific.cpp)
set(ALICE_PLUTOVG_C_SOURCES
    "${ALICE_SRC}/lunasvg/plutovg-blend.c"
    "${ALICE_SRC}/lunasvg/plutovg-canvas.c"
    "${ALICE_SRC}/lunasvg/plutovg-font.c"
    "${ALICE_SRC}/lunasvg/plutovg-ft-math.c"
    "${ALICE_SRC}/lunasvg/plutovg-ft-raster.c"
    "${ALICE_SRC}/lunasvg/plutovg-ft-stroker.c"
    "${ALICE_SRC}/lunasvg/plutovg-matrix.c"
    "${ALICE_SRC}/lunasvg/plutovg-paint.c"
    "${ALICE_SRC}/lunasvg/plutovg-path.c"
    "${ALICE_SRC}/lunasvg/plutovg-rasterize.c"
    "${ALICE_SRC}/lunasvg/plutovg-surface.c"
)

set(ALICE_ENGINE_CPP_SOURCES
    "${DCON_INC}/common_types.cpp"
    "${ALICE_SRC}/common_types/blake2.cpp"
    "${ALICE_SRC}/common_types/date_interface.cpp"
    "${ALICE_SRC}/common_types/prng.cpp"
    "${ALICE_SRC}/common_types/debug_string_convertions.cpp"

    # Map & Graphics
    "${ALICE_SRC}/map/projections.cpp"
    "${ALICE_SRC}/map/map_state.cpp"
    "${ALICE_SRC}/map/map_data_loading.cpp"
    "${ALICE_SRC}/map/map_borders.cpp"
    "${ALICE_SRC}/map/map.cpp"
    "${ALICE_SRC}/map/flow_map.cpp"
    "${ALICE_SRC}/map/map_modes.cpp"
    "${ALICE_SRC}/graphics/opengl_wrapper.cpp"
    "${ALICE_SRC}/graphics/texture.cpp"
    "${ALICE_SRC}/graphics/nine_slice_renderer.cpp"
    "${ALICE_SRC}/graphics/xac.cpp"
    "${ALICE_SRC}/graphics/stateless_render.cpp"
    "${ALICE_SRC}/graphics/asvg.cpp"

    # LunaSVG
    "${ALICE_SRC}/lunasvg/graphics.cpp"
    "${ALICE_SRC}/lunasvg/lunasvg.cpp"
    "${ALICE_SRC}/lunasvg/svgelement.cpp"
    "${ALICE_SRC}/lunasvg/svggeometryelement.cpp"
    "${ALICE_SRC}/lunasvg/svglayoutstate.cpp"
    "${ALICE_SRC}/lunasvg/svgpaintelement.cpp"
    "${ALICE_SRC}/lunasvg/svgparser.cpp"
    "${ALICE_SRC}/lunasvg/svgproperty.cpp"
    "${ALICE_SRC}/lunasvg/svgrenderstate.cpp"
    "${ALICE_SRC}/lunasvg/svgtextelement.cpp"

    # AI
    "${ALICE_SRC}/ai/ai.cpp"
    "${ALICE_SRC}/ai/ai_campaign.cpp"
    "${ALICE_SRC}/ai/ai_campaign_values.cpp"
    "${ALICE_SRC}/ai/ai_focuses.cpp"
    "${ALICE_SRC}/ai/ai_influence.cpp"
    "${ALICE_SRC}/ai/ai_alliances.cpp"
    "${ALICE_SRC}/ai/ai_economy.cpp"
    "${ALICE_SRC}/ai/ai_war.cpp"

    # Economy, Demographics, Culture, Military & Nations
    "${ALICE_SRC}/economy/terms.cpp"
    "${ALICE_SRC}/economy/economy_stats.cpp"
    "${ALICE_SRC}/economy/economy.cpp"
    "${ALICE_SRC}/economy/economy_pops.cpp"
    "${ALICE_SRC}/economy/economy_trade_routes.cpp"
    "${ALICE_SRC}/economy/economy_government.cpp"
    "${ALICE_SRC}/economy/economy_production.cpp"
    "${ALICE_SRC}/economy/construction.cpp"
    "${ALICE_SRC}/economy/advanced_province_buildings.cpp"
    "${ALICE_SRC}/economy/national_budget.cpp"
    "${ALICE_SRC}/economy/trade_network.cpp"
    "${ALICE_SRC}/economy/demographics.cpp"
    "${ALICE_SRC}/economy/pop_merger.cpp"
    "${ALICE_SRC}/culture/culture.cpp"
    "${ALICE_SRC}/culture/politics.cpp"
    "${ALICE_SRC}/culture/rebels.cpp"
    "${ALICE_SRC}/culture/rebel_clustering.cpp"
    "${ALICE_SRC}/military/military.cpp"
    "${ALICE_SRC}/logistics/logistics.cpp"
    "${ALICE_SRC}/nations/nations.cpp"
    "${ALICE_SRC}/provinces/province.cpp"

    # GUI & Topbar Subwindows
    "${ALICE_SRC}/gui/alice_ui.cpp"
    "${ALICE_SRC}/gui/production.cpp"
    "${ALICE_SRC}/gui/province_factories.cpp"
    "${ALICE_SRC}/gui/factory_details.cpp"
    "${ALICE_SRC}/gui/gui.cpp"
    "${ALICE_SRC}/gui/gui_element_types.cpp"
    "${ALICE_SRC}/gui/gui_main_menu.cpp"
    "${ALICE_SRC}/gui/gui_console.cpp"
    "${ALICE_SRC}/gui/gui_event.cpp"
    "${ALICE_SRC}/gui/gui_message_settings_window.cpp"
    "${ALICE_SRC}/gui/gui_message_window.cpp"
    "${ALICE_SRC}/gui/gui_chat_window.cpp"
    "${ALICE_SRC}/gui/gui_combat.cpp"
    "${ALICE_SRC}/gui/gui_tooltips.cpp"
    "${ALICE_SRC}/gui/gui_units.cpp"
    "${ALICE_SRC}/gui/gui_unit_panel_subwindow.cpp"
    "${ALICE_SRC}/gui/gui_diplomacy_request.cpp"
    "${ALICE_SRC}/gui/gui_lookup.cpp"
    "${ALICE_SRC}/gui/gui_leader_tooltip.cpp"
    "${ALICE_SRC}/gui/gui_modifier_tooltips.cpp"
    "${ALICE_SRC}/gui/province_tiles/province_tiles.cpp"
    "${ALICE_SRC}/gui/gui_province_window.cpp"
    "${ALICE_SRC}/gui/gui_trigger_tooltips.cpp"
    "${ALICE_SRC}/gui/gui_error_window.cpp"
    "${ALICE_SRC}/gui/map_tooltip.cpp"
    "${ALICE_SRC}/gui/immediate_mode.cpp"
    "${ALICE_SRC}/gui/economy_viewer.cpp"
    "${ALICE_SRC}/gui/unit_tooltip.cpp"
    "${ALICE_SRC}/gui/gui_context_window.cpp"
    "${ALICE_SRC}/gui/labour_details.cpp"
    "${ALICE_SRC}/gui/gui_common_elements.cpp"
    "${ALICE_SRC}/gui/gui_effect_tooltips.cpp"
    "${ALICE_SRC}/gui/widgets/table.cpp"
    "${ALICE_SRC}/gui/gui_graphics.cpp"
    "${ALICE_SRC}/gui/gui_leaders.cpp"
    "${ALICE_SRC}/gui/ui_state.cpp"
    "${ALICE_SRC}/gui/touch_tooltip_manager.cpp"
    "${ALICE_SRC}/gui/mobile_topbar.cpp"
    "${ALICE_SRC}/gui/mobile_dock_bar.cpp"
    "${ALICE_SRC}/gui/mobile_minimap_drawer.cpp"
    "${ALICE_SRC}/gui/mobile_province_drawer.cpp"
    "${ALICE_SRC}/gui/gui_element_base.cpp"
    "${ALICE_SRC}/gui/gui_other.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/gui_trade_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/gui_population_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/gui_production_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/gui_technology_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/military_subwindows/gui_leaders_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/military_subwindows/gui_stats_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/military_subwindows/gui_units_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/military_subwindows/gui_build_unit_large_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/politics_subwindows/gui_decision_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/politics_subwindows/gui_movements_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/politics_subwindows/gui_reforms_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/politics_subwindows/gui_release_nation_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/politics_subwindows/gui_unciv_reforms_window.cpp"
    "${ALICE_SRC}/gui/topbar_subwindows/production_subwindows/gui_factory_refit_window.cpp"

    # Gamestate, Scripting, Parsing, Text, Network & Zstd
    "${ALICE_SRC}/gamestate/system_state.cpp"
    "${ALICE_SRC}/gamestate/tick_scheduler.cpp"
    "${ALICE_SRC}/gamestate/commands.cpp"
    "${ALICE_SRC}/gamestate/diplomatic_messages.cpp"
    "${ALICE_SRC}/gamestate/modifiers.cpp"
    "${ALICE_SRC}/gamestate/notifications.cpp"
    "${ALICE_SRC}/gamestate/serialization.cpp"
    "${ALICE_SRC}/gamestate/game_scene.cpp"
    "${ALICE_SRC}/gamestate/uitemplate_serialization.cpp"
    "${ALICE_SRC}/gamestate/user_interactions.cpp"
    "${ALICE_SRC}/gamestate/create_windows.cpp"
    "${ALICE_SRC}/gamestate/lua_alice_api.cpp"
    "${GEN_OUT_DIR}/gamestate/dcon_oos_reporter_generated.cpp"
    "${ALICE_SRC}/gamerule/gamerule.cpp"
    "${ALICE_SRC}/scripting/effects.cpp"
    "${ALICE_SRC}/scripting/events.cpp"
    "${ALICE_SRC}/scripting/triggers.cpp"
    "${ALICE_SRC}/scripting/fif_triggers.cpp"
    "${ALICE_SRC}/parsing/float_from_chars.cpp"
    "${ALICE_SRC}/parsing/parsers.cpp"
    "${ALICE_SRC}/text/bmfont.cpp"
    "${ALICE_SRC}/text/fonts.cpp"
    "${ALICE_SRC}/text/text.cpp"
    "${ALICE_SRC}/network/SHA512.cpp"
    "${ALICE_SRC}/network/network.cpp"
    "${ALICE_SRC}/network/pcp.cpp"
    "${ALICE_SRC}/zstd/zstd.cpp"
)

# ==============================================================================
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