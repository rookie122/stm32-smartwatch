@echo off
cd /d "%~dp0"
if exist "..\.venv\Scripts\python.exe" (
  "..\.venv\Scripts\python.exe" -X utf8 tools\build_pc.py --skip-build --run
) else (
  python -X utf8 tools\build_pc.py --skip-build --run
)
if errorlevel 1 pause
