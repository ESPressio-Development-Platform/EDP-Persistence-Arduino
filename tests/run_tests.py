#!/usr/bin/env python3
"""Compile concrete Arduino Persistence providers without invoking PlatformIO orchestration."""

from pathlib import Path
import argparse
import shutil
import subprocess
import sys
import tempfile


def sibling(root, *names):
    for name in names:
        candidate = root.parent / name
        if candidate.is_dir():
            return candidate.resolve()
    return None


def first_existing(*paths):
    for path in paths:
        if path and path.exists():
            return path.resolve()
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler")
    parser.add_argument("--platformio-home")
    parser.add_argument("--persistence")
    parser.add_argument("--system")
    parser.add_argument("--keep-build", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    home = Path(args.platformio_home).expanduser().resolve() if args.platformio_home else Path.home() / ".platformio"
    persistence = Path(args.persistence).resolve() if args.persistence else sibling(root, "EDP-Persistence")
    system = Path(args.system).resolve() if args.system else sibling(root, "EDP-System", "ESPressio-System")
    framework = first_existing(
        home / "packages" / "framework-arduinoespressif32",
    )
    framework_libs = first_existing(
        home / "packages" / "framework-arduinoespressif32-libs" / "esp32",
    )
    compiler = Path(args.compiler).expanduser().resolve() if args.compiler else first_existing(
        home / "packages" / "toolchain-xtensa-esp-elf" / "bin" / "xtensa-esp32-elf-g++",
        home / "packages" / "toolchain-xtensa-esp32" / "bin" / "xtensa-esp32-elf-g++",
    )

    missing = []
    if not persistence: missing.append("sibling EDP-Persistence checkout")
    if not system: missing.append("sibling EDP-System checkout")
    if not framework: missing.append("Arduino-ESP32 framework package under ~/.platformio/packages")
    if not framework_libs: missing.append("Arduino-ESP32 ESP-IDF libraries package under ~/.platformio/packages")
    if not compiler: missing.append("Xtensa ESP32 C++ compiler under ~/.platformio/packages")
    if missing:
        print("ERROR: missing required compile dependency:", file=sys.stderr)
        for item in missing:
            print(f"  - {item}", file=sys.stderr)
        return 2

    build = Path(tempfile.mkdtemp(prefix="edp-persistence-arduino-tests-"))
    try:
        source = root / "tests" / "ContractCompile.cpp"
        object_file = build / "ContractCompile.o"
        includes = [
            root / "src",
            persistence / "src",
            system / "src",
            framework / "cores" / "esp32",
            framework / "variants" / "esp32",
            framework / "libraries" / "FS" / "src",
            framework / "libraries" / "Preferences" / "src",
        ]
        if framework_libs:
            includes.extend(
                path
                for path in (framework_libs / "include").glob("*/include")
                if path.is_dir()
            )
            includes.extend(
                path
                for path in (framework_libs / "include").glob("*")
                if path.is_dir()
            )
        command = [
            str(compiler),
            "-std=gnu++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-DESP32",
            "-DARDUINO_ARCH_ESP32",
            "-DARDUINO=10819",
            "-c",
            str(source),
            "-o",
            str(object_file),
        ]
        for include in includes:
            command.extend(["-I", str(include)])

        print(f"Compiler: {compiler}")
        print(f"Arduino-ESP32: {framework}")
        print(f"Arduino-ESP32 IDF libraries: {framework_libs}")
        print(f"EDP-Persistence-Arduino: {root}")
        print(f"EDP-Persistence: {persistence}")
        print(f"EDP-System: {system}")
        print(f"Build directory: {build}")
        print("\n[1/1] Compiling Arduino concrete contract directly...")

        if args.verbose:
            print(" ".join(command))

        result = subprocess.run(command, check=False)
        if result.returncode != 0:
            print("\nFAIL: Arduino concrete contract did not compile.", file=sys.stderr)
            return result.returncode

        print("\nPASS: Arduino concrete Persistence providers compiled and satisfied the abstract contract.")
        return 0
    finally:
        if args.keep_build:
            print(f"Build retained: {build}")
        else:
            shutil.rmtree(build, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
