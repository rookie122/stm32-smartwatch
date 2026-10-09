"""Download the pinned SDL2 development package and verify its SHA-256."""
from __future__ import annotations

import hashlib
from pathlib import Path
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = "2.30.12"
ARCHIVE = f"SDL2-devel-{VERSION}-VC.zip"
URL = f"https://github.com/libsdl-org/SDL/releases/download/release-{VERSION}/{ARCHIVE}"
SHA256 = "1ca980f9964fb44cf94be235c9818fa1dc8e3f76b08096d751b2d689e5e37d02"


def ensure_sdl() -> Path:
    destination = ROOT / "dependencies"
    package = destination / f"SDL2-{VERSION}"
    required = ("include/SDL.h", "lib/x64/SDL2.lib", "lib/x64/SDL2.dll")
    if all((package / file).is_file() for file in required):
        return package
    downloads = ROOT / "downloads"
    downloads.mkdir(exist_ok=True)
    archive = downloads / ARCHIVE
    if not archive.exists():
        print(f"Downloading SDL2 {VERSION}...", flush=True)
        partial = archive.with_suffix(".zip.part")
        try:
            with urllib.request.urlopen(URL, timeout=60) as response, partial.open("wb") as output:
                while block := response.read(1024 * 1024):
                    output.write(block)
            partial.replace(archive)
        finally:
            partial.unlink(missing_ok=True)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise RuntimeError(f"SDL2 archive checksum mismatch: {archive}. Remove the archive and retry.")
    destination.mkdir(exist_ok=True)
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            resolved = (destination / entry.filename).resolve()
            if not resolved.is_relative_to(destination.resolve()):
                raise RuntimeError("Invalid SDL2 archive entry.")
        source.extractall(destination)
    if not all((package / file).is_file() for file in required):
        raise RuntimeError("SDL2 package is incomplete.")
    print(f"SDL2 {VERSION} ready.")
    return package


if __name__ == "__main__":
    ensure_sdl()
