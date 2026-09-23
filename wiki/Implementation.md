# Private Implementation

FileSystemStorage does not add a lock merely to advertise stronger concurrency; a binding may claim ConcurrentReads only when the complete mounted filesystem and injected dependency configuration genuinely supports it.

Preferences uses a fixed scratch buffer when a truncated caller read requires retrieving a complete native blob first. The namespace must be dedicated because the provider owns its representation conventions.
