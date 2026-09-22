#include <ESPressio_Persistence_Arduino.hpp>

namespace {

    struct ContractBinding final {};

    using ContractFileSystemProfile =
        ESPressio::Persistence::Arduino::FileSystemBindingProfile<
            ESPressio::Persistence::RetentionLevel::Restart,
            ESPressio::Persistence::TextCaseSensitivity::CaseSensitive,
            ESPressio::Persistence::MediaRemovability::Fixed,
            254U,
            254U,
            0xFFFFFFFFULL
        >;


    static_assert([]() consteval {
        ESPressio::Persistence::ValidatePersistenceProvider<
            ESPressio::Persistence::Arduino::FileSystemStorage<ContractBinding, ContractFileSystemProfile>
        >();
        ESPressio::Persistence::ValidatePersistenceProvider<
            ESPressio::Persistence::Arduino::PreferencesKeyValueStorage<ContractBinding>
        >();
        return true;
    }());

} // namespace
