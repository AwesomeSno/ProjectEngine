// =============================================================================
// Project Engine - ECS Query & Chunk Dispatcher
// =============================================================================
// (c) AxeomLabs. All Rights Reserved.
// =============================================================================

#pragma once

#include "engine/ecs/registry.h"
#include "engine/ecs/system_context.h"
#include "engine/threading/job_system.h"
#include "engine/memory/frame_allocator.h"

namespace engine::ecs {

// A lightweight accessor for a specific chunk.
// Passed into the parallel chunk job payload.
struct ChunkView {
    Chunk* chunk;
    
    // Mathematically calculates the offset and casts the array
    template<typename T>
    T* get_array(ComponentID id) const {
        u32 offset = chunk->archetype->get_chunk_offset(id);
        if (offset == 0xFFFFFFFF) return nullptr;
        return chunk->get_array<T>(offset);
    }
};

using ChunkJobFunc = void(*)(ChunkView view, double dt);

struct ChunkJobData {
    Chunk* chunk;
    ChunkJobFunc func;
    double dt;
};

// The execution wrapper that translates the Job System back into ECS logic
inline void internal_chunk_task(void* raw_data, u32, threading::JobCounter*) {
    ChunkJobData* data = static_cast<ChunkJobData*>(raw_data);
    ChunkView view{ data->chunk };
    data->func(view, data->dt);
}

class Query {
public:
    explicit Query(const ComponentMask& mask) : m_mask(mask), m_version(0) {}

    // Core architectural convergence function.
    // Uses a lazy-evaluation cache to completely bypass O(N) archetype scanning.
    // Iterates all chunks matching the mask and instantly saturates the multi-core
    // job system with N chunks. Jobs are bound to the Scheduler's topological barrier.
    void dispatch(SystemContext& ctx, ChunkJobFunc func) {
        u64 current_version = ctx.registry->get_archetype_version();

        // Lazy Cache Invalidation
        if (m_version != current_version) {
            m_cached_archetypes = ctx.registry->get_matching_archetypes(m_mask);
            m_version = current_version;
        }

        // Fast Iteration Path: Zero scanning, pure dispatch
        for (Archetype* arch : m_cached_archetypes) {
            for (Chunk* chunk : arch->chunks) {
                // Must use FrameAllocator because the job data needs to outlive this function scope,
                // but we cannot block the thread with global heap (malloc/new) locks.
                ChunkJobData* job_data = ctx.frame_allocator->allocate<ChunkJobData>();
                job_data->chunk = chunk;
                job_data->func = func;
                job_data->dt = ctx.frame.dt;

                threading::JobDecl job;
                job.task = internal_chunk_task;
                job.data = job_data;

                // Push to the lock-free Work Stealing Queue.
                // Links child to the topological layer counter.
                threading::job_system::dispatch(job, ctx.layer_counter);
            }
        }
    }

private:
    ComponentMask m_mask;
    u64 m_version;
    std::vector<Archetype*> m_cached_archetypes;
};

} // namespace engine::ecs
