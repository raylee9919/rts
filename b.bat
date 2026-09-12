@echo off
setlocal
cd /D "%~dp0"

set preset=debug
set force_configure=
set target=

for %%a in (%*) do (
    if /I "%%a"=="debug" (
        set preset=debug
    ) else if /I "%%a"=="release" (
        set preset=release
    ) else if /I "%%a"=="profile" (
        set preset=profile
    ) else if /I "%%a"=="asan" (
        set preset=asan
    ) else if /I "%%a"=="configure" ( 
        set force_configure=1
    ) else (
        set target=%%a 
    )
)

:: CTIME Begin
if not exist misc mkdir misc
call "util/ctime" -begin misc/rts_build_time.ctm

:: Get cl.exe
where /q cl && goto :have_cl
set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" goto :no_cl
for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "vs_path=%%i"
if not defined vs_path goto :no_cl
call "%vs_path%\VC\Auxiliary\Build\vcvars64.bat" >nul
where /q cl || goto :no_cl
:have_cl

echo [%preset%]

if defined force_configure goto :configure
if exist "build\%preset%\build.ninja" goto :build
:configure
cmake --preset %preset% || goto :fail

:build
set target_arg=
if defined target set target_arg=--target %target%
cmake --build --preset %preset% %target_arg% || goto :fail

:: CTIME End
call "util/ctime" -end misc/rts_build_time.ctm
exit /b 0

:fail
call "util/ctime" -end misc/rts_build_time.ctm
exit /b 1

:no_cl
echo [ERROR]: "cl" not found - install Visual Studio with the C++ workload, or run this from the x64 Native Tools command prompt.
exit /b 1
