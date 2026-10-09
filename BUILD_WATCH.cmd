@echo off
cd /d "%~dp0"
if exist "..\.venv\Scripts\python.exe" (
  "..\.venv\Scripts\python.exe" -X utf8 tools\build_pc.py --run
) else (
  python -X utf8 tools\build_pc.py --run
)
if errorlevel 1 pause
