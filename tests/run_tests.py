#!/usr/bin/env python3
from pathlib import Path
import argparse, shutil, subprocess, sys, tempfile

def sibling(root, *names):
    for name in names:
        p = root.parent / name
        if p.is_dir(): return p.resolve()
    return None

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--pio"); ap.add_argument("--persistence"); ap.add_argument("--system"); ap.add_argument("--keep-build",action="store_true"); ap.add_argument("--verbose",action="store_true"); a=ap.parse_args()
    root=Path(__file__).resolve().parents[1]
    persistence=Path(a.persistence).resolve() if a.persistence else sibling(root,"EDP-Persistence")
    system=Path(a.system).resolve() if a.system else sibling(root,"EDP-System","ESPressio-System")
    pio=a.pio or shutil.which("pio") or shutil.which("platformio")
    if not pio or not persistence or not system:
        print("ERROR: require PlatformIO plus sibling EDP-Persistence and EDP-System (or pass explicit paths).",file=sys.stderr); return 2
    build=Path(tempfile.mkdtemp(prefix="edp-persistence-arduino-tests-"))
    try:
        (build/"src").mkdir(); shutil.copy2(root/"tests"/"ContractCompile.cpp",build/"src"/"main.cpp")
        (build/"platformio.ini").write_text(f"""[platformio]
default_envs = contract
[env:contract]
platform = espressif32
framework = arduino
board = esp32dev
build_flags =
    -std=gnu++20
    -I{root/"src"}
    -I{persistence/"src"}
    -I{system/"src"}
build_unflags =
    -std=gnu++11
    -std=gnu++14
    -std=gnu++17
""")
        print(f"PlatformIO: {pio}\nEDP-Persistence-Arduino: {root}\nEDP-Persistence: {persistence}\nEDP-System: {system}\nBuild directory: {build}\n\n[1/1] Compiling Arduino concrete contract...")
        cmd=[pio,"run","-d",str(build)]+(["-v"] if a.verbose else [])
        rc=subprocess.run(cmd,check=False).returncode
        print("\nPASS: Arduino concrete Persistence providers compiled and satisfied the abstract contract." if rc==0 else "\nFAIL: Arduino concrete contract did not compile.")
        return rc
    finally:
        if a.keep_build: print(f"Build retained: {build}")
        else: shutil.rmtree(build,ignore_errors=True)
if __name__=="__main__": raise SystemExit(main())
