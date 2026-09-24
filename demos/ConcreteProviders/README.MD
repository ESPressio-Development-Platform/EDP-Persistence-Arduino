# Concrete Providers Demo

This demo exercises both concrete Arduino Persistence providers with real platform storage and an explicit EDP-Memory ByteOperations provider.

The demo selects `EDP-Platform-Portable::Memory::ByteOperationsProvider` as the standard C/C++ implementation of `EDP-Memory::ByteOperations`.

## Hardware

Use an ESP32 target with an SD card connected to the board's default Arduino `SD` pins. The key/value portion uses the board's normal NVS/Preferences partition.

## Dependencies

Install or provide:

- EDP-Persistence-Arduino;
- EDP-Persistence;
- EDP-Memory;
- EDP-Platform;
- EDP-Platform-Portable;
- EDP-System;
- Arduino-ESP32.

## Targets

- `Arduino_IDE` — open `ConcreteProviders.ino` after installing the dependencies above.
  The sketch uses the root-level `ESPressio_Platform_Portable_ByteOperations.hpp` entry point so Arduino can discover the optional provider library without importing unrelated portable providers.
- `PlatformIO_Arduino` — uses the maintained PIOArduino platform package and C++20.
- `PlatformIO_ESP-IDF` — uses Arduino as an ESP-IDF component through the maintained PIOArduino platform package and C++20.

See [../../docs/BUILDING.MD](../../docs/BUILDING.MD) for the exact C++20/PIOArduino baseline.

## Demonstrated semantics

- `FileSystemStorage` uses an SD card through Arduino `SD`.
- The demo filesystem profile advertises restart retention, case-insensitive FAT paths, removable media, a 63-byte path limit, a 31-byte segment limit, a 4096-byte maximum logical file, and `ConcurrentReads`. The concurrency guarantee is an explicit binding assertion for this FAT-backed mount; arbitrary `fs::FS` bindings still default to `CallerSerialized`.
- `PreferencesKeyValueStorage` stores, reads, replaces with a present zero-length value, queries that value, and removes the key.
- Both providers receive the same caller-owned ByteOperations provider.

## ISR boundary

The demo deliberately performs all Persistence work from ordinary execution context. These providers are not ISR-safe and must not be called from ISR context.
