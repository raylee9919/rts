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

if "%game%"=="1" set BuildGame=1
if "%win%"=="1"  set BuildWin=1
if "%gl%"=="1"   set BuildGL=1

if not defined BuildGame if not defined BuildWin if not defined BuildGL (
    set "BuildGame=1"
    set "BuildWin=1"
    set "BuildGL=1"
    echo building all..
)

set compiler=cl
set flags_common=-std:c++17 -nologo -FC -Zi -EHsc- -utf-8 -D__DEVELOPER=1 -I..\src -I..\src\vendor
set flags_debug=/Od /DBUILD_DEBUG=1
set flags_release=/O2 /DBUILD_DEBUG=0
set flags_warning=/W4 /D_CRT_SECURE_NO_WARNINGS /wd4456 /wd4100 /wd4189 /wd4505 /wd4201 /wd4477 /wd4311 /wd4302 /wd4005 /wd4244 /wd4706
set flags_linker=/incremental:no /opt:ref

:: Choose Compile/Link Lines
                        set flags_compile=%flags_common% %flags_warning%
if "%debug%"=="1"       set flags_compile=%flags_compile% %flags_debug%
if "%release%"=="1"     set flags_compile=%flags_compile% %flags_release%
if "%profile%"=="1"     set flags_compile=%flags_compile% /DBUILD_PROFILE=1    && echo [Profiler Enabled]
if "%asan%"=="1"        set flags_compile=%flags_compile% /fsanitize=address && echo [ASAN Enabled]


:: ---------------------------- Projects ---------------------------- ::
if not exist build mkdir build
pushd build

REM if exist *.pdb del *.pdb

:: ---------------------------- Tools ---------------------------- ::
:: Assimp
REM call %compiler% %flags_compile% ..\src\rts_assimp.cpp -Fe:assimp.exe -I../src/vendor -link %flags_linker% ..\lib\assimp-vc143-mt.lib .ss.\lib\meshoptimizer.lib

:: Metaprogramming
REM call %compiler% ..\src\meta\rts_meta.cpp /Fe:rts_meta.exe %flags_compile% /link %flags_linker%
REM rts_meta.exe

:: ---------------------------- Build ---------------------------- ::
call rc /nologo /fo logo.res ..\data\logo.rc || exit /b 1

:: Renderers
if "%BuildGL%"=="1" (
    call %compiler% %flags_compile% ..\src\rts_win32_opengl.cpp /Fe:rts_renderer_opengl /LD /link %flags_linker% /PDB:win32_opengl_%random%.pdb
)

:: Game
if "%BuildGame%"=="1" (
    call %compiler% %flags_compile% ..\src\rts.cpp       /Fe:rts_game /LD /link %flags_linker% /PDB:game_%random%.pdb
)

:: Platform
if "%BuildWin%"=="1" (
    call %compiler% %flags_compile% ..\src\rts_win32.cpp /Fe:rts          /link %flags_linker% logo.res
)


del *.obj *.res >nul
popd

:: CTIME End
call "util/ctime" -end misc/rts_build_time.ctm
rem call "util/ctime" -stats misc/rts_build_time.ctm
