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
    missing = []
    if not pio: missing.append("PlatformIO executable ('pio' or 'platformio')")
    if not persistence: missing.append("sibling EDP-Persistence checkout")
    if not system: missing.append("sibling EDP-System checkout")
    if missing:
        print("ERROR: missing required test dependency:", file=sys.stderr)
        for item in missing:
            print(f"  - {item}", file=sys.stderr)
        if not pio:
            print("\nPlatformIO is not available on PATH. If it is installed elsewhere, run:", file=sys.stderr)
            print("  python3 tests/run_tests.py --pio /path/to/pio", file=sys.stderr)
        if not persistence:
            print("\nExpected EDP-Persistence beside this repository, or pass --persistence /path/to/EDP-Persistence.", file=sys.stderr)
        if not system:
            print("\nExpected EDP-System beside this repository, or pass --system /path/to/EDP-System.", file=sys.stderr)
        return 2
    build=Path(tempfile.mkdtemp(prefix="edp-persistence-arduino-tests-"))
    try:
        (build/"src").mkdir(); shutil.copy2(root/"tests"/"ContractCompile.cpp",build/"src"/"main.cpp")
        (build/"platformio.ini").write_text(f"""[platformio]
default_envs = contract
[env:contract]
platform = espressif32
framework = arduino
board = esp32dev
lib_ldf_mode = deep+
build_flags =
    -std=gnu++20
    -I{root/"src"}
    -I{persistence/"src"}
    -I{system/"src"}
    -I$PROJECT_PACKAGES_DIR/framework-arduinoespressif32/libraries/FS/src
    -I$PROJECT_PACKAGES_DIR/framework-arduinoespressif32/libraries/Preferences/src
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
