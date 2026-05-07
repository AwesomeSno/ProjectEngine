// =============================================================================
// Project Engine - ECS Types
// =============================================================================

#pragma once

#include "engine/core/types.h"

namespace engine::ecs {

using Entity = u32;
using ComponentID = u8; // Supports up to 256 unique component types

struct ComponentOps {
    void (*construct)(void* ptr);
    void (*destruct)(void* ptr);
    void (*move)(void* dst, void* src); // Moves from src to dst, potentially destroying src
    void (*copy)(void* dst, const void* src); // Optional for add_component
};

enum class ComponentFlags : u32 {
    None              = 0,
    Serializable      = 1 << 0,
    Snapshotable      = 1 << 1,
    NetworkReplicated = 1 << 2,
    EditorVisible     = 1 << 3,
    Deterministic     = 1 << 4
};

// Enables bitwise operations on ComponentFlags
inline ComponentFlags operator|(ComponentFlags a, ComponentFlags b) {
    return static_cast<ComponentFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline ComponentFlags operator&(ComponentFlags a, ComponentFlags b) {
    return static_cast<ComponentFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}
inline bool has_flag(ComponentFlags value, ComponentFlags flag) {
    return (value & flag) != ComponentFlags::None;
}

struct ComponentInfo {
    ComponentID id;
    const char* name;
    ComponentFlags flags;

    u32 size;
    u32 alignment;
    u32 stride;

    ComponentOps ops;
};

constexpr Entity NULL_ENTITY = 0xFFFFFFFF;

// 32-bit Entity: 20 bits for Index (1 million active), 12 bits for Generation (ABA safety)
constexpr u32 entity_index(Entity e) { return e & 0xFFFFF; }
constexpr u32 entity_gen(Entity e) { return e >> 20; }
constexpr Entity make_entity(u32 index, u32 gen) { return (gen << 20) | (index & 0xFFFFF); }

} // namespace engine::ecs
