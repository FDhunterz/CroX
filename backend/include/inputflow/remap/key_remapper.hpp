#pragma once

#include <vector>
#include "inputflow/types.hpp"
#include "inputflow/input/i_input_provider.hpp"

namespace inputflow {

class KeyRemapper {
public:
    explicit KeyRemapper(IInputProvider& input);

    void setBindings(const std::vector<KeyBinding>& bindings);
    bool tryRemap(const KeyEvent& event);

private:
    IInputProvider& input_;
    std::vector<KeyBinding> bindings_;
};

}  // namespace inputflow
