@echo off
setlocal
cd /d "%~dp0"

call devenv "build\rts.exe"
