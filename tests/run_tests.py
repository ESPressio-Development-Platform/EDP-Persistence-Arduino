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
    parser.add_argument("--host-compiler")
    parser.add_argument("--platformio-home")
    parser.add_argument("--persistence")
    parser.add_argument("--memory")
    parser.add_argument("--platform")
    parser.add_argument("--platform-portable")
    parser.add_argument("--system")
    parser.add_argument("--keep-build", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    home = Path(args.platformio_home).expanduser().resolve() if args.platformio_home else Path.home() / ".platformio"
    persistence = Path(args.persistence).resolve() if args.persistence else sibling(root, "EDP-Persistence")
    memory = Path(args.memory).resolve() if args.memory else sibling(root, "EDP-Memory")
    platform = Path(args.platform).resolve() if args.platform else sibling(root, "EDP-Platform")
    platform_portable = (
        Path(args.platform_portable).resolve()
        if args.platform_portable
        else sibling(root, "EDP-Platform-Portable")
    )
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
    host_compiler_value = (
        args.host_compiler
        or shutil.which("c++")
        or shutil.which("clang++")
        or shutil.which("g++")
    )
    host_compiler = (
        Path(host_compiler_value).expanduser().resolve()
        if host_compiler_value
        else None
    )

    missing = []
    if not persistence: missing.append("sibling EDP-Persistence checkout")
    if not memory: missing.append("sibling EDP-Memory checkout")
    if not platform: missing.append("sibling EDP-Platform checkout")
    if not platform_portable: missing.append("sibling EDP-Platform-Portable checkout")
    if not system: missing.append("sibling EDP-System checkout")
    if not framework: missing.append("Arduino-ESP32 framework package under ~/.platformio/packages")
    if not framework_libs: missing.append("Arduino-ESP32 ESP-IDF libraries package under ~/.platformio/packages")
    if not compiler: missing.append("Xtensa ESP32 C++ compiler under ~/.platformio/packages")
    if not host_compiler: missing.append("host C++ compiler (c++, clang++, or g++)")
    if missing:
        print("ERROR: missing required compile dependency:", file=sys.stderr)
        for item in missing:
            print(f"  - {item}", file=sys.stderr)
        return 2

    build = Path(tempfile.mkdtemp(prefix="edp-persistence-arduino-tests-"))
    try:
        behavior_source = root / "tests" / "ProviderBehaviorTests.cpp"
        behavior_executable = build / "ProviderBehaviorTests"
        behavior_command = [
            str(host_compiler),
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-I",
            str(root / "tests" / "support" / "arduino"),
            "-I",
            str(root / "src"),
            "-I",
            str(persistence / "src"),
            "-I",
            str(memory / "src"),
            "-I",
            str(platform / "src"),
            "-I",
            str(platform_portable / "src"),
            "-I",
            str(system / "src"),
            str(behavior_source),
            "-o",
            str(behavior_executable),
        ]

        print(f"Host compiler: {host_compiler}")
        print("\n[1/4] Compiling and running concrete provider behavior tests...")

        if args.verbose:
            print(" ".join(str(value) for value in behavior_command))

        result = subprocess.run(
            behavior_command,
            check=False,
        )

        if result.returncode != 0:
            print(
                "\nFAIL: Arduino concrete provider behavior tests did not compile.",
                file=sys.stderr,
            )
            return result.returncode

        result = subprocess.run(
            [str(behavior_executable)],
            check=False,
        )

        if result.returncode != 0:
            print(
                "\nFAIL: Arduino concrete provider behavior tests failed.",
                file=sys.stderr,
            )
            return result.returncode

        demo_sources = (
            (
                "[2/4] Compiling PlatformIO Arduino demo source...",
                root / "demos" / "ConcreteProviders" / "PlatformIO_Arduino" / "src" / "main.cpp",
                build / "DemoPlatformIOArduino.o",
            ),
            (
                "[3/4] Compiling PlatformIO ESP-IDF demo source...",
                root / "demos" / "ConcreteProviders" / "PlatformIO_ESP-IDF" / "src" / "main.cpp",
                build / "DemoPlatformIOEspIdf.o",
            ),
        )

        for label, demo_source, demo_object in demo_sources:
            print(f"\n{label}")
            demo_command = [
                str(host_compiler),
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-I",
                str(root / "tests" / "support" / "arduino"),
                "-I",
                str(root / "src"),
                "-I",
                str(persistence / "src"),
                "-I",
                str(memory / "src"),
                "-I",
                str(platform / "src"),
                "-I",
                str(platform_portable / "src"),
                "-I",
                str(system / "src"),
                "-c",
                str(demo_source),
                "-o",
                str(demo_object),
            ]

            if args.verbose:
                print(" ".join(str(value) for value in demo_command))

            result = subprocess.run(
                demo_command,
                check=False,
            )

            if result.returncode != 0:
                print(
                    "\nFAIL: Arduino concrete provider demo source did not compile.",
                    file=sys.stderr,
                )
                return result.returncode

        source = root / "tests" / "ContractCompile.cpp"
        object_file = build / "ContractCompile.o"
        includes = [
            root / "src",
            persistence / "src",
            memory / "src",
            platform / "src",
            platform_portable / "src",
            system / "src",
            framework / "cores" / "esp32",
            framework / "variants" / "esp32",
            framework / "libraries" / "FS" / "src",
            framework / "libraries" / "Preferences" / "src",
        ]
        if framework_libs:
            include_root = framework_libs / "include"
            includes.append(include_root)
            includes.extend(
                path
                for path in include_root.rglob("include")
                if path.is_dir()
            )
            freertos_headers = list(framework_libs.rglob("freertos/FreeRTOS.h"))
            for header in freertos_headers:
                component_include = header.parent.parent.parent
                if component_include.is_dir():
                    includes.append(component_include)
            for config_header in framework_libs.rglob("FreeRTOSConfig.h"):
                config_include = config_header.parent
                if config_include.is_dir():
                    includes.append(config_include)
            sdkconfig_headers = list(framework_libs.rglob("sdkconfig.h"))
            if not sdkconfig_headers:
                print("ERROR: sdkconfig.h was not found in the installed Arduino-ESP32 libraries package.", file=sys.stderr)
                return 2
            for sdkconfig_header in sdkconfig_headers:
                sdkconfig_include = sdkconfig_header.parent
                if sdkconfig_include.is_dir():
                    includes.append(sdkconfig_include)
            # Arduino-ESP32 3.x packages ESP-IDF as many component include
            # trees, including target-specific directories such as soc/esp32.
            # Add every directory named "include" recursively so the direct
            # compiler sees the same component header roots as the SDK build.
            includes.extend(
                include_dir
                for include_dir in framework_libs.rglob("include")
                if include_dir.is_dir()
            )
            # Some IDF target headers are generated/packaged below directories
            # that are not themselves named "include" (for example soc headers).
            # Derive an include root from every header path whose suffix starts
            # with a known public namespace directory.
            # Add only canonical component roots plus the handful of
            # generated/target-specific header directories that are not rooted
            # beneath an "include" directory. Avoid one -I per header directory:
            # macOS has a finite argv size and the IDF header corpus exceeds it.
            required_headers = (
                "freertos/FreeRTOS.h",
                "FreeRTOSConfig.h",
                "portmacro.h",
                "sdkconfig.h",
                "soc/reg_base.h",
                "esp_newlib.h",
                "rom/ets_sys.h",
            )
            for required_header in required_headers:
                matches = list(framework_libs.rglob(required_header))
                if not matches:
                    print(f"ERROR: {required_header} was not found in the installed Arduino-ESP32 libraries package.", file=sys.stderr)
                    return 2
                for header in matches:
                    if "/" in required_header:
                        suffix_parts = Path(required_header).parts
                        include_root = header
                        for _ in suffix_parts:
                            include_root = include_root.parent
                        includes.append(include_root)
                    else:
                        includes.append(header.parent)
            portmacro_headers = list(framework_libs.rglob("portmacro.h"))
            if not portmacro_headers:
                print("ERROR: portmacro.h was not found in the installed Arduino-ESP32 libraries package.", file=sys.stderr)
                return 2
            for portmacro_header in portmacro_headers:
                portmacro_include = portmacro_header.parent
                if portmacro_include.is_dir():
                    includes.append(portmacro_include)
        command = [
            str(compiler),
            "-std=gnu++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            # Arduino-ESP32/ESP-IDF deliberately uses GCC extensions such as
            # #include_next. Keep strict warnings for EDP code, but do not let
            # pedantic diagnostics originating in the SDK fail this probe.
            "-Wno-pedantic",
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
        print(f"EDP-Memory: {memory}")
        print(f"EDP-Platform: {platform}")
        print(f"EDP-Platform-Portable: {platform_portable}")
        print(f"EDP-System: {system}")
        print(f"Build directory: {build}")
        print("\n[4/4] Compiling Arduino concrete contract directly...")

        if args.verbose:
            print(" ".join(command))

        result = subprocess.run(command, check=False)
        if result.returncode != 0:
            print("\nFAIL: Arduino concrete contract did not compile.", file=sys.stderr)
            return result.returncode

        print(
            "\nPASS: Arduino concrete provider behavior tests and demo source "
            "validation passed, and the providers compiled against the real SDK contract."
        )
        return 0
    finally:
        if args.keep_build:
            print(f"Build retained: {build}")
        else:
            shutil.rmtree(build, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
