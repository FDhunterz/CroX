#pragma once

#include <set>
#include <mutex>
#include "inputflow/types.hpp"
#include "inputflow/input/i_input_provider.hpp"

namespace inputflow {

/// Tracks keys sent as down; ensures up on cancel/shutdown to prevent stuck keys.
class KeyReleaseGuard {
public:
    explicit KeyReleaseGuard(IInputProvider& input);

    void trackDown(KeyCode key, Modifier mods);
    void trackUp(KeyCode key, Modifier mods);
    void releaseAll();

private:
    struct HeldKey {
        KeyCode key;
        Modifier mods;
        bool operator<(const HeldKey& o) const;
    };

    IInputProvider& input_;
    std::mutex mutex_;
    std::set<HeldKey> held_;
};

}  // namespace inputflow
