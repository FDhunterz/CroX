#pragma once

#include "inputflow/input/i_input_provider.hpp"

namespace inputflow {

/// Future extension point for HID / gamepad / virtual device providers.
/// Implementations must remain user-consented and use official OS APIs only.
class IHidInputProvider : public IInputProvider {
public:
    virtual bool enumerateDevices() = 0;
    virtual std::string deviceId() const = 0;
};

}  // namespace inputflow
