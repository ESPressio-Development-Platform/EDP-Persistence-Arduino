# Architecture

`FileSystemStorage<TBindingTag,TBindingProfile,TByteOps>` wraps an externally mounted Arduino filesystem. It never owns mount/format/unmount lifecycle. `PreferencesKeyValueStorage<TBindingTag,TByteOps>` wraps one dedicated Preferences namespace.

A BindingTag creates distinct selectable provider types. Binding profiles statically describe facts the mounted filesystem must genuinely satisfy, including retention, case behaviour, removability, path/file limits and invocation concurrency.
