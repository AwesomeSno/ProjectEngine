// =============================================================================
// Project Engine - ECS Chunk (Memory Foundation)
// =============================================================================

#pragma once

#include "engine/core/types.h"
#include "engine/ecs/types.h"

namespace engine::ecs {

struct Archetype;

constexpr u32 CHUNK_SIZE = 16 * 1024;      // 16KB strictly aligns with L1/L2 cache
constexpr u32 CHUNK_HEADER_SIZE = 16; 
constexpr u32 CHUNK_DATA_SIZE = CHUNK_SIZE - CHUNK_HEADER_SIZE;

// A Chunk holds pure Struct of Arrays (SoA) data for exactly one Archetype.
// No pointers allowed between components.
struct alignas(64) Chunk {
    Archetype* archetype;
    u32 count;
    u32 capacity;
    u32 padding; // Keeps 'data' 64-byte aligned
    
    alignas(64) u8 data[CHUNK_DATA_SIZE];

    // Helper to retrieve a contiguous typed array from the raw data block
    template<typename T>
    T* get_array(u32 byte_offset) {
        return reinterpret_cast<T*>(data + byte_offset);
    }
};

static_assert(sizeof(Chunk) == CHUNK_SIZE, "Chunk must be exactly 16KB to prevent cache line tearing.");

} // namespace engine::ecs
