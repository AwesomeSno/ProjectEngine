// =============================================================================
// Project Engine - ECS Registry (Data Master)
// =============================================================================

#pragma once

#include "engine/ecs/types.h"
#include "engine/ecs/archetype.h"
#include <vector>
#include <new>
#include <utility>

namespace engine::ecs {

// Provides O(1) lookup to find exactly where an Entity lives in memory
struct EntityLocation {
    Archetype* archetype = nullptr;
    u32 chunk_index = 0;
    u32 row_index = 0;
};

class Registry {
public:
    template<typename T>
    void register_component(ComponentID id, const char* name, u32 custom_stride = sizeof(T)) {
        ComponentInfo info;
        info.id = id;
        info.name = name;
        info.size = sizeof(T);
        info.alignment = alignof(T);
        info.stride = custom_stride;

        ComponentOps ops;
        ops.construct = [](void* ptr) { new (ptr) T(); };
        ops.destruct = [](void* ptr) { static_cast<T*>(ptr)->~T(); };
        ops.move = [](void* dst, void* src) { 
            new (dst) T(std::move(*static_cast<T*>(src))); 
        };
        ops.copy = [](void* dst, const void* src) {
            new (dst) T(*static_cast<const T*>(src));
        };
        info.ops = ops;
        m_component_info[id] = info;
    }

    // Exposes component metadata for runtime reflection
    const ComponentInfo& get_component_info(ComponentID id) const { return m_component_info[id]; }

    // Exposes current archetype count. Queries use this to know if new archetypes were added.
    u32 get_archetype_count() const { return static_cast<u32>(m_archetypes.size()); }
    
    // Retrieves an archetype by exact index for O(1) Query cache updates.
    Archetype* get_archetype(u32 index) const { return m_archetypes[index]; }

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
    Archetype* find_or_create_archetype(const ComponentMask& mask);
    void migrate_entity(Entity entity, Archetype* new_archetype);
    void swap_remove(EntityLocation loc);

    std::vector<Archetype*> m_archetypes;
    ComponentInfo m_component_info[256]; // Component metadata and lifecycle functions

    // Sparse set: Direct array mapped to entity_index(Entity)
    // Ensures any entity lookup takes 1 memory access
    std::vector<EntityLocation> m_entity_locations;
    
    // List of recycled entity indices
    std::vector<u32> m_free_entities;
};

} // namespace engine::ecs
