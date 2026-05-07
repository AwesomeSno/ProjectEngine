// =============================================================================
// Project Engine - ECS Archetype
// =============================================================================

#pragma once

#include "engine/ecs/types.h"
#include "engine/ecs/system_descriptor.h" // For ComponentMask
#include "engine/ecs/chunk.h"
#include <vector>

namespace engine::ecs {

// Defines where a specific component array lives within a Chunk
struct ComponentLayout {
    ComponentID id;
    u32 size;         // Logical size of the component
    u32 alignment;    // SIMD alignment requirement (e.g., 16, 32, 64)
    u32 stride;       // Explicit byte stride per element (size + potential padding)
    u32 chunk_offset; // Byte offset into Chunk::data, guaranteed to match `alignment`
};

// Represents a specific combination of components
class Archetype {
public:
    ComponentMask mask;
    u32 entities_per_chunk;
    
    // Kept sorted by ComponentID for fast O(log N) offset lookup
    std::vector<ComponentLayout> layouts;
    std::vector<Chunk*> chunks;

    u32 get_chunk_offset(ComponentID id) const {
        for (const auto& layout : layouts) {
            if (layout.id == id) return layout.chunk_offset;
        }
        return 0xFFFFFFFF;
    }
};

} // namespace engine::ecs
