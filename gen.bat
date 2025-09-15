@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

:: Unpack Arguments.
for %%a in (%*) do set "%%a=1"
if "%debug%"=="1" set release=0 && echo [Font]
if "%model%"=="1" set model=0   && echo [Model]

:: Generate
if "%font%"=="1"   call "build/rts_font_provider.exe"
if "%model%"=="1"  call "build/font_assimp.exe"
