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

struct ArchetypeJobData {
    Archetype* arch;
    ChunkJobFunc func;
    SystemContext* ctx;
};

inline void internal_archetype_task(void* raw_data, u32, threading::JobCounter*) {
    ArchetypeJobData* data = static_cast<ArchetypeJobData*>(raw_data);
    SystemContext& ctx = *data->ctx;
    
    // Worker Thread Fan-Out: Subdivides the archetype into parallel chunk jobs
    for (Chunk* chunk : data->arch->chunks) {
        ChunkJobData* job_data = ctx.frame_allocator->allocate<ChunkJobData>();
        job_data->chunk = chunk;
        job_data->func = data->func;
        job_data->dt = ctx.frame.dt;

        threading::JobDecl job;
        job.task = internal_chunk_task;
        job.data = job_data;

        threading::job_system::dispatch(job, ctx.layer_counter);
    }
}

class Query {
public:
    explicit Query(const ComponentMask& mask) : m_mask(mask), m_evaluated_archetypes(0) {}

    // Core architectural convergence function.
    // Uses continuous incremental caching to evaluate ONLY newly created archetypes,
    // completely bypassing O(N) archetype rescanning.
    void dispatch(SystemContext& ctx, ChunkJobFunc func) {
        u32 current_count = ctx.registry->get_archetype_count();

        // Additive Cache Evaluation (O(1) amortized, only checks newly built archetypes)
        if (m_evaluated_archetypes < current_count) {
            for (u32 i = m_evaluated_archetypes; i < current_count; ++i) {
                Archetype* arch = ctx.registry->get_archetype(i);
                
                // Check if the new archetype perfectly matches this query
                bool matches = true;
                for (int bit = 0; bit < 4; ++bit) {
                    if ((arch->mask.bits[bit] & m_mask.bits[bit]) != m_mask.bits[bit]) {
                        matches = false;
                        break;
                    }
                }
                
                if (matches) {
                    m_cached_archetypes.push_back(arch);
                }
            }
            m_evaluated_archetypes = current_count;
        }

        // Hierarchical Job Dispatch:
        // Main thread only dispatches Archetype jobs (O(A) where A is matching archetypes).
        // Worker threads then fan out into Chunk jobs (O(C) where C is chunks per archetype).
        for (Archetype* arch : m_cached_archetypes) {
            ArchetypeJobData* job_data = ctx.frame_allocator->allocate<ArchetypeJobData>();
            job_data->arch = arch;
            job_data->func = func;
            job_data->ctx = &ctx;

            threading::JobDecl job;
            job.task = internal_archetype_task;
            job.data = job_data;

            // Push Archetype job to WSQ. It links to the layer_counter.
            // When the Archetype job runs, it pushes Chunk jobs also linked to layer_counter.
            threading::job_system::dispatch(job, ctx.layer_counter);
        }
    }

private:
    ComponentMask m_mask;
    u32 m_evaluated_archetypes;
    std::vector<Archetype*> m_cached_archetypes;
};

} // namespace engine::ecs
