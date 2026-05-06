// =============================================================================
// Project Engine - System Descriptor
// =============================================================================
// (c) AxeomLabs. All Rights Reserved.
// =============================================================================
// The non-negotiable execution contract for ECS Systems.
// Acts as the bridge between the ECS, Job System, and Frame Contract.
// =============================================================================

#pragma once

#include "engine/core/types.h"

namespace engine::ecs {

// Represents a bitmask of component dependencies.
// Used by the Job System to automatically resolve Read/Write data races.
struct ComponentMask {
    u64 bits[4]{0}; // Supports up to 256 unique component types

    constexpr void set(u32 component_id) {
        bits[component_id / 64] |= (1ULL << (component_id % 64));
    }
    
    constexpr bool test(u32 component_id) const {
        return (bits[component_id / 64] & (1ULL << (component_id % 64))) != 0;
    }
    
    constexpr bool overlaps(const ComponentMask& other) const {
        return (bits[0] & other.bits[0]) || 
               (bits[1] & other.bits[1]) || 
               (bits[2] & other.bits[2]) || 
               (bits[3] & other.bits[3]);
    }
};

namespace engine::threading { struct JobCounter; }

struct SystemContext {
    void* registry;
    double delta_time;
};

using SystemID = u32;

// Compile-time FNV-1a hash for deterministic System ID generation
constexpr SystemID hash_system(const char* str) {
    SystemID hash = 2166136261u;
    while (*str) {
        hash ^= static_cast<SystemID>(*str++);
        hash *= 16777619u;
    }
    return hash;
}

enum class Phase : u8 {
    Input,
    Simulation,
    Animation,
    RenderPrep,
    Render
};

enum class Frequency : u8 {
    Fixed120Hz,
    Fixed30Hz,
    Variable
};

enum class DataFreshness : u8 {
    Persistent, // Stale data across frames is valid (e.g., 30Hz Pathfinding for 120Hz steering)
    PerTick     // Requires fresh data. If parent skips, dependent MUST also skip.
};

struct SystemDependency {
    SystemID id;
    DataFreshness freshness;
};

struct SystemDesc {
    SystemID id;

    Phase phase;
    Frequency frequency;

    ComponentMask reads;
    ComponentMask writes;

    // Scheduling: Explicit semantic ordering
    const SystemDependency* depends_on;
    u32 depends_count;

    // Execution: The system MUST iterate its chunks and dispatch Job System tasks using ctx.layer_counter.
    void (*execute)(SystemContext& ctx);
};

} // namespace engine::ecs
