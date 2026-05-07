// =============================================================================
// Project Engine - Core Handle Definition
// =============================================================================

#pragma once

#include "engine/core/types.h"

namespace engine::core {

// A strongly-typed, ABA-safe generation handle.
// Prevents raw pointers from crossing system boundaries or lingering across snapshots.
template<typename T>
struct Handle {
    u32 index;
    u32 generation;

    bool is_valid() const {
        return index != 0xFFFFFFFF && generation != 0;
    }

    bool operator==(const Handle<T>& other) const {
        return index == other.index && generation == other.generation;
    }

    bool operator!=(const Handle<T>& other) const {
        return !(*this == other);
    }
};

template<typename T>
inline Handle<T> null_handle() {
    return Handle<T>{0xFFFFFFFF, 0};
}

} // namespace engine::core
