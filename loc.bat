@echo off
setlocal
cd /d "%~dp0"

call .\util\cloc-2.10.exe .\src "--exclude-dir=ThirdParty
