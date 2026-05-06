// =============================================================================
// Project Engine - Frame Allocator (Stub for Compilation)
// =============================================================================

#pragma once
#include "engine/core/types.h"
#include <utility>

namespace engine::memory {

// Lock-free per-thread bump allocator.
// Memory allocated here lives only until the end of the frame, when the offset is reset.
// Prevents OS-level mutex locking on new/malloc during tight loops.
class FrameAllocator {
public:
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        // TEMPORARY STUB: Will be replaced by actual lock-free bump offset logic.
        return new T(std::forward<Args>(args)...);
    }
};

} // namespace engine::memory
