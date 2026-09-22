#include <memory/ByteOperationsProvider.hpp>

#include <ESPressio_Persistence_Arduino.hpp>

namespace {

    namespace Framework = ESPressio::System::CompositionFramework;

    /// Logical provider identity used by the contract probe.
    struct ContractBinding final {};


    // Concrete contract-probe types.

    /// Standard C/C++ ByteOperations provider selected for architecture validation.
    using ContractByteOperationsProvider =
        ESPressio::Platform::Portable::Memory::ByteOperationsProvider;

    /// Conservative filesystem profile used only for compile-time contract validation.
    using ContractFileSystemProfile =
        ESPressio::Persistence::Arduino::FileSystemBindingProfile<
            ESPressio::Persistence::RetentionLevel::Restart,
            ESPressio::Persistence::TextCaseSensitivity::CaseSensitive,
            ESPressio::Persistence::MediaRemovability::Fixed,
            254U,
            254U,
            0xFFFFFFFFULL
        >;

    /// Concrete FileStorage provider type validated by this probe.
    using ContractFileStorageProvider =
        ESPressio::Persistence::Arduino::FileSystemStorage<
            ContractBinding,
            ContractFileSystemProfile,
            ContractByteOperationsProvider
        >;

    /// Concrete KeyValueStorage provider type validated by this probe.
    using ContractKeyValueStorageProvider =
        ESPressio::Persistence::Arduino::PreferencesKeyValueStorage<
            ContractBinding,
            ContractByteOperationsProvider
        >;

    /// Memory-domain Composition supplying the cross-domain ByteOperations dependency.
    using ContractMemoryComposition = Framework::Composition<
        ESPressio::Memory::Domain,
        ContractByteOperationsProvider
    >;

    /// Persistence-domain Composition containing both concrete providers.
    using ContractPersistenceComposition = Framework::Composition<
        ESPressio::Persistence::Domain,
        ContractFileStorageProvider,
        ContractKeyValueStorageProvider
    >;

    /// Complete architecture proving the Persistence -> Memory dependency is satisfied.
    using ContractArchitecture = Framework::Architecture<
        ContractMemoryComposition,
        ContractPersistenceComposition
    >;


    static_assert(ContractArchitecture::IsValid);

    static_assert([]() consteval {
        ESPressio::Persistence::ValidatePersistenceProvider<
            ContractFileStorageProvider
        >();
        ESPressio::Persistence::ValidatePersistenceProvider<
            ContractKeyValueStorageProvider
        >();
        return true;
    }());

} // namespace
