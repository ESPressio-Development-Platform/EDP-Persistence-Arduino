# Resources, Lifecycle and Concurrency

No mount lifecycle is owned. File-provider failure preservation is MayModify with interruption atomicity determined by its declared profile/implementation. Preferences individual mutations advertise power-loss atomicity where the native store supports it; ClearAll has weaker semantics. General operations are not ISR-safe.
