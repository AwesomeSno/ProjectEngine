// =============================================================================
// Project Engine - System Context
// =============================================================================

#pragma once

namespace engine::ecs {

#include "engine/core/types.h"

namespace engine::threading { struct JobCounter; }
namespace engine::memory { class FrameAllocator; }
namespace engine::ecs { class Registry; class CommandBuffer; class EventBus; }

namespace engine::ecs {

struct FrameInfo {
    u32 tick_index;        // Deterministic simulation tick
    double dt;             // Fixed delta time or variable dt depending on Phase
    double alpha;          // Interpolation alpha (only valid for RenderPrep/Render phases)
};

// The non-negotiable architectural boundary for all Systems.
// Prevents systems from bypassing the architecture to grab global state.
struct SystemContext {
    // 1. ECS Access (Query views and chunk iteration)
    Registry* registry;
    
    // 2. Timing
    FrameInfo frame;

    // 3. Allocator Access (Scratch space)
    // Prevents systems from calling new/malloc. Lock-free frame lifetime.
    memory::FrameAllocator* frame_allocator;

    // 4. Job Dispatch Handle
    // Systems MUST dispatch their N*Chunks jobs attached to this counter.
    threading::JobCounter* layer_counter;

    // 5. Deferred Structural Changes
    // Systems MUST use this to add/remove components or destroy entities.
    CommandBuffer* commands;

    // 6. Deterministic Event Streams
    // Systems MUST append events here instead of triggering immediate callbacks.
    EventBus* events;
};

} // namespace engine::ecs
