@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

:: CTIME Begin
if not exist misc mkdir misc
call "util/ctime" -begin misc/rts_build_time.ctm

:: Get cl.exe
where cl >nul 2>nul
if %errorlevel%==1 (
    echo Looking for 'vcvars64.bat'.. Recommended to run from the Developer Command Prompt.
    @call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

where /q cl || (
    echo [ERROR]: "cl" not found - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)


:: Unpack Arguments.
for %%a in (%*) do set "%%a=1"
if not "%release%"=="1" set debug=1
if "%debug%"=="1" set release=0 && echo [Debug Build]
if "%release%"=="1" set debug=0 && echo [Release Build]

if "%fbx%"=="1"    set build_fbx=1
if "%assimp%"=="1" set build_assimp=1
if "%game%"=="1"   set BuildGame=1
if "%os%"=="1"     set BuildWin=1
if "%gl%"=="1"     set BuildGL=1

if not defined build_assimp if not defined build_fbx if not defined BuildGame if not defined BuildWin if not defined BuildGL (
    set "BuildGame=1"
    set "BuildWin=1"
    set "BuildGL=1"
    echo building all..
)

set compiler=cl
set flags_common=-std:c++17 -nologo -FC -Zi -EHsc- -utf-8 -D__DEVELOPER=1 -I..\src -I..\src\vendor
set flags_debug=/Od /DBUILD_DEBUG=1
set flags_release=/O2 /DBUILD_DEBUG=0
:: 4100: unreferenced formal parameter
:: 4189: local variable is initialized but not referenced
:: 4456: declaration hides previous local declaration
:: 4244::::::::::::::::::::::::::::::::::::::::::::::::::
set flags_warning=/W4 /D_CRT_SECURE_NO_WARNINGS -wd4201 -wd4505 -wd4100 -wd4189 -wd4244
set flags_linker=/incremental:no /opt:ref

:: Choose Compile/Link Lines
                        set flags_compile=%flags_common% %flags_warning%
if "%debug%"=="1"       set flags_compile=%flags_compile% %flags_debug%
if "%release%"=="1"     set flags_compile=%flags_compile% %flags_release%
if "%profile%"=="1"     set flags_compile=%flags_compile% -DBUILD_PROFILE=1  && echo [Profiler Enabled]
if "%asan%"=="1"        set flags_compile=%flags_compile% -fsanitize=address && echo [ASAN Enabled]


:: ---------------------------- Projects ---------------------------- ::
if not exist build mkdir build
pushd build

REM if exist *.pdb del *.pdb

:: ---------------------------- Tools ---------------------------- ::
:: Assimp
if "%assimp%" == "1" (
    call %compiler% %flags_compile% ..\src\rts_assimp.cpp -Fe:assimp.exe -I../src/vendor -link %flags_linker% ..\lib\assimp-vc143-mtd.lib
)

:: FBX
if "%fbx%" == "1" (
    call %compiler% %flags_compile% ..\src\importer\fbx_importer.cpp -Fe:fbx.exe -I../src/third_party/ufbx -link %flags_linker%
)

:: Metaprogramming
REM call %compiler% ..\src\meta\rts_meta.cpp /Fe:rts_meta.exe %flags_compile% /link %flags_linker%
REM rts_meta.exe

:: ---------------------------- Build ---------------------------- ::
call rc /nologo /fo logo.res ..\data\logo.rc || exit /b 1

:: Renderers
if "%BuildGL%"=="1" (
    call %compiler% %flags_compile% ..\src\rts_win32_opengl.cpp /Fe:rts_renderer_opengl -I../src/third_party/opengl /LD /link opengl32.lib %flags_linker% /PDB:win32_opengl_%random%.pdb
)

:: Game
if "%BuildGame%"=="1" (
    call %compiler% %flags_compile% ..\src\game.cpp /Fe:rts_game /LD /link %flags_linker% /PDB:game_%random%.pdb /EXPORT:game_update_and_render
)

:: Platform
if "%BuildWin%"=="1" (
    call %compiler% %flags_compile% ..\src\rts_win32.cpp /Fe:rts /link %flags_linker% logo.res
)


del *.obj *.res >nul
popd

:: CTIME End
call "util/ctime" -end misc/rts_build_time.ctm
rem call "util/ctime" -stats misc/rts_build_time.ctm
