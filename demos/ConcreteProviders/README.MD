# Concrete Providers Demo

This demo exercises both concrete Arduino Persistence providers with real platform storage:

- `FileSystemStorage` uses an SD card through Arduino `SD`. The binding profile deliberately advertises conservative guarantees: restart retention, case-insensitive FAT paths, removable media, a 63-byte path limit, a 31-byte segment limit, and a 4096-byte maximum logical file.
- `PreferencesKeyValueStorage` stores, reads, replaces with a present zero-length value, queries that zero-length value, and removes the key.

## Hardware

Use an ESP32 target with an SD card connected to the board's default Arduino `SD` pins. The key/value portion uses the board's normal NVS/Preferences partition.

## Targets

- `Arduino_IDE` — install EDP-Persistence-Arduino, EDP-Persistence and EDP-System as Arduino libraries, then open `ConcreteProviders.ino`.
- `PlatformIO_Arduino` — Arduino-ESP32 target.
- `PlatformIO_ESP-IDF` — ESP-IDF target with Arduino included as an ESP-IDF component through PlatformIO's `framework = arduino, espidf` mode.

All variants use C++20.
