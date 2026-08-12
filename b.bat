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
if "%rts%"=="1"    set build_rts=1
if "%test%"=="1"   set build_test=1

if not defined build_fbx if not defined build_rts if not defined test (
    set "build_rts=1"
    echo building all..
)

set compiler=cl
set flags_common=/std:c++17 /nologo /FC /Zi /EHsc- /utf-8 /I..\src
set flags_debug=/Od /DBUILD_DEBUG=1
set flags_release=/O2 /DBUILD_DEBUG=0
:: 4100: unreferenced formal parameter
:: 4189: local variable is initialized but not referenced
:: 4456: declaration hides previous local declaration
:: 4244::::::::::::::::::::::::::::::::::::::::::::::::::
set flags_warning=/W4 /D_CRT_SECURE_NO_WARNINGS -wd4201 -wd4505 -wd4100 -wd4189 -wd4244 -wd4127
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
:: FBX
if "%fbx%" == "1" (
    call %compiler% %flags_compile% ..\src\importer\fbx_importer.cpp ..\src\ThirdParty\meshoptimizer\*.cpp -Fe:fbx.exe -I../src/ThirdParty/ufbx -link %flags_linker%
)

:: Metaprogramming
REM call %compiler% ..\src\meta\rts_meta.cpp /Fe:rts_meta.exe %flags_compile% /link %flags_linker%
REM rts_meta.exe

:: ---------------------------- Build ---------------------------- ::
call rc /nologo /fo logo.res ..\data\logo.rc || exit /b 1

:: RTS
if "%build_rts%"=="1" (
    call %compiler% %flags_compile% -I../src/ThirdParty/opengl ..\src\rts.cpp /Fe:rts /link opengl32.lib %flags_linker% logo.res
)

:: Test
if "%build_test%"=="1" (
    REM call %compiler% %flags_compile% ..\src\Test\test_ds.cpp  /Fe:test_ds  /link %flags_linker%
    call %compiler% %flags_compile% ..\src\Test\test_rhi.cpp /Fe:test_rhi /link %flags_linker%
    REM call %compiler% %flags_compile% ..\src\Test\test_thread.cpp /Fe:test_thread /link %flags_linker%
)

del *.obj *.res >nul
popd

:: CTIME End
call "util/ctime" -end misc/rts_build_time.ctm
rem call "util/ctime" -stats misc/rts_build_time.ctm
