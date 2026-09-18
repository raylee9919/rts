cmake_minimum_required(VERSION 3.25)

project(rts LANGUAGES C CXX)

if (NOT MSVC)
    message(FATAL_ERROR "rts builds with MSVC (cl) only.")
endif()

option(RTS_PROFILE "Enable Tracy/PIX instrumentation (BUILD_PROFILE=1)" OFF)
option(RTS_ASAN    "Build with AddressSanitizer"                        OFF)
set(RTS_OUTPUT_SUFFIX "" CACHE STRING "Suffix appended to executable names")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")

set(CMAKE_C_FLAGS_DEBUG            "/Od")
set(CMAKE_CXX_FLAGS_DEBUG          "/Od")
set(CMAKE_C_FLAGS_RELEASE          "/O2")
set(CMAKE_CXX_FLAGS_RELEASE        "/O2")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG   "/DEBUG /INCREMENTAL:NO /OPT:REF")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /OPT:REF")
if (RTS_PROFILE)
    set(CMAKE_C_FLAGS_RELEASE          "/Od")
    set(CMAKE_CXX_FLAGS_RELEASE        "/Od")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /OPT:NOREF,NOICF")
endif()
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

include(CheckIPOSupported)
check_ipo_supported(RESULT rts_ipo_supported LANGUAGES C CXX)
if (rts_ipo_supported AND NOT RTS_PROFILE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")


add_library(rts_options INTERFACE)
target_include_directories(rts_options INTERFACE "${CMAKE_SOURCE_DIR}/src")
target_include_directories(rts_options INTERFACE "${CMAKE_SOURCE_DIR}/data/shaders")
target_compile_definitions(rts_options INTERFACE
    NOMINMAX
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
    BUILD_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
)
target_compile_options(rts_options INTERFACE
    /utf-8
    /FC
    $<$<COMPILE_LANGUAGE:CXX>:/EHsc->
)
if (RTS_PROFILE)
    target_compile_definitions(rts_options INTERFACE BUILD_PROFILE=1 TRACY_ENABLE)
    target_link_directories(rts_options INTERFACE "${CMAKE_SOURCE_DIR}/dll")
endif()
if (RTS_ASAN)
    target_compile_options(rts_options INTERFACE /fsanitize=address)
endif()

add_library(rts_warnings INTERFACE)
target_compile_options(rts_warnings INTERFACE
    /W3 /WX /wd4201 /wd4505 /wd4100 /wd4189 /wd4244 /wd4127
)


add_library(rts_third_party STATIC
    src/third_party/xxhash3/xxhash.c
)
if (RTS_PROFILE)
    target_sources(rts_third_party PRIVATE src/third_party/tracy/TracyClient.cpp)
endif()
target_link_libraries(rts_third_party PUBLIC rts_options)


add_library(rts_engine STATIC
    src/basic/allocator.cpp
    src/basic/arena.cpp
    src/basic/context.cpp
    src/basic/core.cpp
    src/basic/hash.cpp
    src/basic/log.cpp
    src/basic/string.cpp
    src/basic/string_builder.cpp

    src/math/math.cpp

    src/os/os.cpp
    src/os/win32/win32.cpp

    src/random/random.cpp

    src/geometry/cdt.cpp
    src/geometry/geogen.cpp

    src/rect_pack/rect_pack.cpp

    src/asset/parser.cpp
    src/asset/system.cpp
    src/asset/texture.cpp

    src/rhi/rhi.cpp
    src/rhi/d3d12/rhi_d3d12.cpp

    src/gfx/gfx.cpp

    src/renderer/renderer.cpp

    src/shader_compiler/slang/slang.cpp

    src/asset/asset.cpp

    src/text_file_handler/text_file_handler.cpp

    src/audio/audio.cpp

    src/game.cpp
    src/shared.cpp
    src/material.cpp
)
target_link_libraries(rts_engine
    PUBLIC  rts_options rts_third_party "${CMAKE_SOURCE_DIR}/src/third_party/slang/lib/slang.lib"
    PRIVATE rts_warnings
)
target_precompile_headers(rts_engine PRIVATE "${CMAKE_SOURCE_DIR}/src/pch.h")


file(GLOB rts_runtime_dlls CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/dll/*.dll")

function(rts_add_executable name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE rts_engine rts_warnings)
    target_precompile_headers(${name} REUSE_FROM rts_engine)
    set_target_properties(${name} PROPERTIES OUTPUT_NAME "${name}${RTS_OUTPUT_SUFFIX}")
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${rts_runtime_dlls} "$<TARGET_FILE_DIR:${name}>"
        VERBATIM
    )
endfunction()

rts_add_executable(rts    src/main.cpp)


add_custom_target(compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    VERBATIM
)
