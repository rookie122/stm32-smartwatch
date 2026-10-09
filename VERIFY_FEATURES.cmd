@echo off
cd /d "%~dp0"
if exist "..\.venv\Scripts\python.exe" (
  "..\.venv\Scripts\python.exe" -X utf8 tools\verify_features.py --build
) else (
  python -X utf8 tools\verify_features.py --build
)
pause
