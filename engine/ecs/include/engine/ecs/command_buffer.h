// =============================================================================
// Project Engine - ECS Command Buffer
// =============================================================================
#pragma once

#include "engine/ecs/types.h"
#include <vector>

namespace engine::ecs {

class Registry;

enum class CommandType : u8 {
    AddComponent,
    RemoveComponent,
    DestroyEntity
};

struct Command {
    CommandType type;
    Entity entity;
    ComponentID component_id;
    void* payload; // Payload pointer (must be valid until flush)
    u32 payload_size;
};

// Records deferred structural changes.
// Flushed at the end of a topological layer or phase.
class CommandBuffer {
public:
    void add_component(Entity entity, ComponentID id, void* data, u32 size) {
        m_commands.push_back({CommandType::AddComponent, entity, id, data, size});
    }

    void remove_component(Entity entity, ComponentID id) {
        m_commands.push_back({CommandType::RemoveComponent, entity, id, nullptr, 0});
    }

    void destroy_entity(Entity entity) {
        m_commands.push_back({CommandType::DestroyEntity, entity, 0, nullptr, 0});
    }

    void flush(Registry* registry);

private:
    std::vector<Command> m_commands;
};

} // namespace engine::ecs
