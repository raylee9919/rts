@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

call "util/cloc.exe" src --exclude-dir="vendor"
