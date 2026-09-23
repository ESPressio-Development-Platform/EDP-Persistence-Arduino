# Public API

FileSystemStorage implements mandatory file operations plus directories, enumeration, rename, append and ranged write. PreferencesKeyValueStorage implements key/value operations within Arduino's native key constraints.

Preferences values are bounded to the provider's supported size. Native Arduino Preferences cannot directly store a zero-length `putBytes` value, so the provider uses a reserved representation marker to preserve the semantic distinction between an absent key and a present empty value.

Exact declarations remain authoritative in the exported headers.
