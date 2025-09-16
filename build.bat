@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

where /q cl || (
    echo [ERROR]: "cl" not found - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)

:: Unpack Arguments.
for %%a in (%*) do set "%%a=1"
if not "%release%"=="1" set debug=1
if "%debug%"=="1" set release=0 && echo [Debug Build]
if "%release%"=="1" set debug=0 && echo [Release Build]

set compiler=cl
set flags_common=/std:c++17 /nologo /FC /Z7 /utf-8 /D__DEVELOPER=1 /D__MSVC=1 /I..\src /I..\src\vendor
set flags_debug=/Od /DBUILD_DEBUG=1
set flags_release=/O2 /DBUILD_DEBUG=0
set flags_warning=/W4 /D_CRT_SECURE_NO_WARNINGS /wd4456 /wd4100 /wd4189 /wd4505 /wd4201 /wd4477 /wd4311 /wd4302 /wd4005 /wd4244 /wd4706
set flags_linker=/incremental:no /opt:ref

:: Choose Compile/Link Lines
                        set flags_compile=%flags_common% %flags_warning%
if "%debug%"=="1"       set flags_compile=%flags_compile% %flags_debug%
if "%release%"=="1"     set flags_compile=%flags_compile% %flags_release%


:: ---------------------------- Project ---------------------------- ::
if not exist build mkdir build
pushd build

if exist *.pdb del *.pdb

:: Font
rem call %compiler% %flags_compile% ..\src\font\rts_font_provider.cpp /link %flags_linker% gdi32.lib 

:: Assimp
rem call %compiler% %flags_compile% ..\src\rts_assimp.cpp /I../src/vendor /link %flags_linker% ..\lib\assimp-vc143-mt.lib

:: Metaprogramming
call %compiler% ..\src\meta\rts_meta.cpp /Fe:rts_meta.exe %flags_compile% /link %flags_linker%
rts_meta.exe

:: Renderers
set renderer_export=/EXPORT:win32_load_renderer /EXPORT:win32_begin_frame /EXPORT:win32_end_frame
call %compiler% %flags_compile% ..\src\win32_opengl.cpp /LD /link %flags_linker% /PDB:win32_opengl_%random%.pdb %renderer_export%

:: Build Icon
call rc /nologo /fo logo.res ..\data\logo.rc || exit /b 1

:: Game
call %compiler% %flags_compile% ..\src\rts_game.cpp  /Fe:rts_game /LD /link %flags_linker% /PDB:game_%random%.pdb /EXPORT:game_update_and_render
call %compiler% %flags_compile% ..\src\rts_win32.cpp /Fe:rts          /link %flags_linker% logo.res

popd
