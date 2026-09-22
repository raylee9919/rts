@echo off
setlocal
cd /D "%~dp0"

:: Get cl.exe
where /q cl && goto :have_cl
set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" goto :no_cl
for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "vs_path=%%i"
if not defined vs_path goto :no_cl
call "%vs_path%\VC\Auxiliary\Build\vcvars64.bat" >nul
where /q cl || goto :no_cl
:have_cl

if not exist build mkdir build
pushd build

set defines=/DNOMINMAX /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS /DBUILD_DEBUG=1
set includes=/I../src /I../data
set sources=../src/build.cpp ../src/shared.cpp ../src/material_codegen.cpp ^
            ../src/basic/allocator.cpp ../src/basic/arena.cpp ../src/basic/context.cpp ^
            ../src/basic/core.cpp ../src/basic/hash.cpp ../src/basic/log.cpp ^
            ../src/basic/string.cpp ../src/basic/string_builder.cpp ^
            ../src/math/math.cpp ^
            ../src/os/os.cpp ../src/os/win32/win32.cpp ^
            ../src/shader_compiler/slang/slang.cpp ^
            ../src/third_party/xxhash3/xxhash.c

call cl /nologo /std:c++17 /Od /Zi /utf-8 /FC /EHsc- %defines% %includes% /Fe:build.exe %sources% ^
     /link ../src/third_party/slang/lib/slang.lib || goto :fail

popd
exit /b 0

:fail
popd
exit /b 1

:no_cl
echo [ERROR]: "cl" not found.
exit /b 1
