@echo off
setlocal
cd /d "%~dp0"

REM call .\util\cloc-2.10.exe .\src "--exclude-dir=third_party
call .\util\cloc-2.10.exe .\src"
