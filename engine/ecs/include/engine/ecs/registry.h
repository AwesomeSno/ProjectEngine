// =============================================================================
// Project Engine - ECS Registry (Data Master)
// =============================================================================

#pragma once

#include "engine/ecs/types.h"
#include "engine/ecs/archetype.h"
#include <vector>

namespace engine::ecs {

// Provides O(1) lookup to find exactly where an Entity lives in memory
struct EntityLocation {
    Archetype* archetype = nullptr;
    u32 chunk_index = 0;
    u32 row_index = 0;
};

class Registry {
public:
    // Filters all archetypes against the bitmask to find perfectly matched component groups
    std::vector<Archetype*> get_matching_archetypes(const ComponentMask& query_mask) const {
        std::vector<Archetype*> result;
        for (Archetype* arch : m_archetypes) {
            bool matches = true;
            for (int i = 0; i < 4; ++i) {
                // The archetype must contain all bits specified in the query mask
                if ((arch->mask.bits[i] & query_mask.bits[i]) != query_mask.bits[i]) {
                    matches = false;
                    break;
                }
            }
            if (matches) result.push_back(arch);
        }
        return result;
    }

    // Structural Changes (Called only during safe flush windows, never during queries)
    void add_component(Entity entity, ComponentID id, void* data, u32 size);
    void remove_component(Entity entity, ComponentID id);
    void destroy_entity(Entity entity);

private:
    // Migration Pipeline
    void migrate_entity(Entity entity, Archetype* new_archetype);
    void swap_remove(EntityLocation loc);

    std::vector<Archetype*> m_archetypes;

    // Sparse set: Direct array mapped to entity_index(Entity)
    // Ensures any entity lookup takes 1 memory access
    std::vector<EntityLocation> m_entity_locations;
    
    // List of recycled entity indices
    std::vector<u32> m_free_entities;
};

} // namespace engine::ecs
