@echo off
setlocal
cd /D "%~dp0"

if not exist build mkdir build
pushd build

call cl /nologo /Od /Zi /W3 /WX /wd4996 /I../src /I../src/third_party/llvm/include ../src/meta.cpp ../src/third_party/llvm/lib/libclang.lib /Fe:meta.exe

popd
