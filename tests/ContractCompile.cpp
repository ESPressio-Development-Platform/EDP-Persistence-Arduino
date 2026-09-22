#include <ESPressio_Persistence_Arduino.hpp>

namespace {

    struct ContractBinding final {};


    static_assert([]() consteval {
        ESPressio::Persistence::ValidatePersistenceProvider<
            ESPressio::Persistence::Arduino::FileSystemStorage<ContractBinding>
        >();
        ESPressio::Persistence::ValidatePersistenceProvider<
            ESPressio::Persistence::Arduino::PreferencesKeyValueStorage<ContractBinding>
        >();
        return true;
    }());

} // namespace
