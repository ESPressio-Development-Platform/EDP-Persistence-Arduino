# Build Baseline

## C++20

EDP-Persistence-Arduino requires official C++20.

This requirement comes from the EDP-System Composition Framework and applies to the concrete Persistence providers, their selected EDP-Memory ByteOperations provider, and consuming applications.

## Recommended Arduino toolchain: PIOArduino

For Arduino-ESP32 builds, use the maintained PIOArduino platform package.

The supplied PlatformIO-compatible demo uses:

```ini
[env:esp32dev]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = esp32dev
framework = arduino

build_unflags =
    -std=gnu++11

build_flags =
    -std=gnu++20
```

This is the supported baseline for the repository demos and is the recommended configuration for new consumers.

## Using the PlatformIO Core executable

Developers who retain the conventional `platformio` command may still select the PIOArduino platform package explicitly through the same `platform = https://github.com/pioarduino/.../platform-espressif32.zip` setting above.

Do not rely on an unpinned/default `platform = espressif32` Arduino environment and assume it supplies an appropriate Arduino-ESP32/C++20 toolchain.

The important requirements are:

1. explicitly select the maintained PIOArduino platform package;
2. remove any inherited `-std=gnu++11` flag;
3. add `-std=gnu++20`;
4. compile all EDP dependencies with the same C++20 baseline.

## Arduino as an ESP-IDF component

The supplied `PlatformIO_ESP-IDF` demo uses:

```ini
framework = arduino, espidf
```

with the same maintained PIOArduino platform package and C++20 flags.

## Arduino IDE

Arduino IDE consumers must use an Arduino-ESP32 core/toolchain release with C++20 support and install the required EDP libraries:

- EDP-Persistence-Arduino;
- EDP-Persistence;
- EDP-Memory;
- EDP-Platform;
- a concrete ByteOperations provider such as EDP-Platform-Portable;
- EDP-System.

The consuming sketch must compile under C++20.
