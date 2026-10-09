"""Build the standalone OV-Watch desktop application using installed VS tools."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from setup_dependencies import ensure_sdl

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
NATIVE = BUILD / "native"


def visual_studio() -> Path:
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    install = subprocess.check_output([
        str(vswhere), "-latest", "-products", "*", "-requires",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath",
    ], text=True).strip()
    if not install:
        raise RuntimeError("Visual Studio C++ build tools are required.")
    return Path(install)


def build() -> Path:
    ensure_sdl()
    install = visual_studio()
    cmake = shutil.which("cmake") or str(install / "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe")
    ninja = shutil.which("ninja") or str(install / "Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
    BUILD.mkdir(parents=True, exist_ok=True)
    commands = [
        [cmake, "-S", str(ROOT), "-B", str(NATIVE), "-G", "Ninja",
         f"-DCMAKE_MAKE_PROGRAM={ninja}", "-DCMAKE_BUILD_TYPE=Release"],
        [cmake, "--build", str(NATIVE), "--parallel", "4"],
    ]
    batch = BUILD / "compile.cmd"
    lines = ["@echo off", f'call "{install / "VC/Auxiliary/Build/vcvars64.bat"}" >nul',
             "if errorlevel 1 exit /b 1"]
    for command in commands:
        lines += [subprocess.list2cmdline(command), "if errorlevel 1 exit /b 1"]
    batch.write_text("\n".join(lines) + "\n", encoding="utf-8")
    with (BUILD / "build.log").open("w", encoding="utf-8") as log:
        print("Building OV-Watch with MSVC and CMake/Ninja...", flush=True)
        result = subprocess.run(["cmd.exe", "/d", "/c", str(batch)], stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((BUILD / "build.log").read_text(encoding="utf-8", errors="replace")[-14000:])
        raise RuntimeError(f"Build failed. See {BUILD / 'build.log'}")
    executable = NATIVE / "bin/ov_watch.exe"
    print(f"Build succeeded: {executable}")
    return executable


def launch(executable: Path) -> int:
    log = (BUILD / "watch.log").open("w", encoding="utf-8")
    try:
        process = subprocess.Popen(
            [str(executable)], cwd=executable.parent,
            env={**os.environ,"OV_WATCH_DATA_DIR":str(ROOT / "data")},
            stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
    finally:
        log.close()
    (BUILD / "last_launch.json").write_text(json.dumps({"pid": process.pid, "executable": str(executable)}, indent=2), encoding="utf-8")
    print(f"OV-Watch window launched, PID {process.pid}")
    return process.pid


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    try:
        executable = NATIVE / "bin/ov_watch.exe"
        if not args.skip_build or not executable.exists():
            executable = build()
        if args.run:
            launch(executable)
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(str(exc), file=sys.stderr)
        sys.exit(1)
