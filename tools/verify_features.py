"""Verify core boundaries, malformed storage, actual LVGL controls, and process restart."""
from __future__ import annotations
import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import zlib
from build_pc import ROOT, NATIVE, build


def verify() -> Path:
    run=ROOT / "build/runs" / dt.datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    run.mkdir(parents=True)
    state=run / "app-data"
    state.mkdir()
    executable=NATIVE / "bin/ov_watch.exe"
    core=NATIVE / "watch_core_tests.exe"
    environment={**os.environ,"SDL_VIDEODRIVER":"dummy","SDL_AUDIODRIVER":"dummy",
                 "OV_WATCH_DATA_DIR":str(state)}
    records=[]

    def execute(name: str,command: list[str]) -> str:
        result=subprocess.run(command,cwd=run,env=environment,stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",timeout=30)
        (run / f"{name}.log").write_text(result.stdout,encoding="utf-8")
        cases=[line.removeprefix("PASS ") for line in result.stdout.splitlines() if line.startswith("PASS ")]
        records.append({"stage":name,"exit_code":result.returncode,"passed":result.returncode==0,
                        "cases":cases,"command":command,"log":str(run / f"{name}.log")})
        print(result.stdout,flush=True)
        if result.returncode:
            raise RuntimeError(f"{name} failed: exit {result.returncode:#x}; see {run}")
        return result.stdout

    execute("core",[str(core)])
    raw=(run / "state.bin").read_bytes()
    assert zlib.crc32(raw[:-4])==struct.unpack("<I",raw[-4:])[0]

    def changed(offset: int,value: bytes) -> bytes:
        payload=bytearray(raw[:-4]);payload[offset:offset+len(value)]=value
        return bytes(payload)+struct.pack("<I",zlib.crc32(payload))

    fixtures={
        "bit_corruption":raw[:100]+bytes([raw[100]^128])+raw[101:],
        "truncated_file":raw[:-7],
        "unexpected_trailing_data":raw+b"unexpected",
        "unsupported_version":changed(4,struct.pack("<H",99)),
        "invalid_threshold_with_valid_crc":changed(6,struct.pack("<H",0)),
        "oversized_count_with_valid_crc":changed(39,struct.pack("<H",121)),
        "invalid_date_with_valid_crc":changed(15,struct.pack("<I",20260230)),
        "invalid_boolean_with_valid_crc":changed(14,b"\x02"),
        "invalid_alert_sequence_with_valid_crc":changed(43+120*7,struct.pack("<I",999)),
    }
    for name,contents in fixtures.items():
        path=run / f"{name}.bin";path.write_bytes(contents)
        execute(name,[str(core),"--reject",str(path)])
    execute("ui",[str(executable),"--verify-ui",str(run)])
    execute("process_restart",[str(executable),"--verify-restore"])
    screenshots=[]
    try:
        from PIL import Image
        for bitmap in sorted(run.glob("*.bmp")):
            png=bitmap.with_suffix(".png")
            with Image.open(bitmap) as shot: shot.save(png)
            screenshots.append(str(png))
    except ImportError:
        screenshots=[str(path) for path in sorted(run.glob("*.bmp"))]
    memory_line=next(line for line in (run / "ui.log").read_text(encoding="utf-8").splitlines()
                     if line.startswith("UI_MEMORY before_free="))
    memory={key:int(value) for key,value in re.findall(r"(\w+)=(\d+)",memory_line)}
    report={"created_at_local":dt.datetime.now().isoformat(),"run_directory":str(run),
            "scope":"Native Windows C core; headless SDL + real LVGL control events; no hardware",
            "all_passed":all(item["passed"] for item in records),
            "passed_cases":sum(len(item["cases"]) for item in records),
            "stages":records,"screenshots":screenshots,
            "executable_sha256":hashlib.sha256(executable.read_bytes()).hexdigest(),
            "page_roundtrips":40,"lvgl_heap_observation":memory,
            "memory_scope":"Stable live block and screen counts; free payload changes with fragmentation/fit; not a hardware RAM measurement or long-run leak proof",
            "state_bytes":(state / "watch_state.bin").stat().st_size}
    destination=run / "report.json"
    destination.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    print(f"Verified {report['passed_cases']} cases. Report: {destination}")
    return destination


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build",action="store_true")
    args=parser.parse_args()
    if args.build: build()
    verify()
