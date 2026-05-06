// =============================================================================
// Project Engine - Lock-Free Work Stealing Queue
// =============================================================================
// (c) AxeomLabs. All Rights Reserved.
// =============================================================================
// A lock-free Chase-Lev deque. 
// Single Producer (pushes/pops from the bottom).
// Multiple Consumers (steals from the top).
// =============================================================================

#pragma once

#include "engine/core/types.h"
#include "engine/core/compiler.h"
#include <atomic>

namespace engine::threading {

struct JobCounter; // Forward declaration

// Internal Job representation within the queues
struct Job {
    void (*task)(void* data, u32 thread_index, JobCounter* parent) = nullptr;
    void* data = nullptr;
    JobCounter* counter = nullptr;
};

class WorkStealingQueue {
public:
    // Capacity must be a power of 2. 131072 easily holds 50k jobs/frame without resizing.
    WorkStealingQueue(u32 capacity = 131072) 
        : m_capacity(capacity), m_mask(capacity - 1) {
        ENGINE_STATIC_ASSERT((131072 & (131072 - 1)) == 0, "Capacity must be a power of 2");
        m_jobs = new Job[capacity];
        m_top.store(0, std::memory_order_relaxed);
        m_bottom.store(0, std::memory_order_relaxed);
    }

    ~WorkStealingQueue() {
        delete[] m_jobs;
    }

    // Producer pushes to the bottom
    void push(const Job& job) {
        i64 b = m_bottom.load(std::memory_order_relaxed);
        m_jobs[b & m_mask] = job;
        
        // Ensure job is written before bottom is updated
        std::atomic_thread_fence(std::memory_order_release);
        m_bottom.store(b + 1, std::memory_order_relaxed);
    }

    // Producer pops from the bottom
    bool pop(Job& out_job) {
        i64 b = m_bottom.load(std::memory_order_relaxed) - 1;
        m_bottom.store(b, std::memory_order_relaxed);
        
        std::atomic_thread_fence(std::memory_order_seq_cst);
        i64 t = m_top.load(std::memory_order_relaxed);

        if (t <= b) {
            out_job = m_jobs[b & m_mask];
            if (t != b) {
                // More than one item left, safe to return
                return true;
            }
            // Exactly one item left, we might race with a stealer
            if (!m_top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                out_job.task = nullptr; // Stealer won
                m_bottom.store(t + 1, std::memory_order_relaxed);
                return false;
            }
            m_bottom.store(t + 1, std::memory_order_relaxed);
            return true;
        } else {
            // Empty
            m_bottom.store(t, std::memory_order_relaxed);
            return false;
        }
    }

    // Consumer (other thread) steals from the top
    bool steal(Job& out_job) {
        i64 t = m_top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        i64 b = m_bottom.load(std::memory_order_acquire);

        if (t < b) {
            out_job = m_jobs[t & m_mask];
            if (m_top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }

private:
    Job* m_jobs;
    u32 m_capacity;
    u32 m_mask;
    
    // Aligned to cache lines to prevent false sharing between producer and stealers
    ENGINE_CACHE_ALIGN std::atomic<i64> m_top;
    ENGINE_CACHE_ALIGN std::atomic<i64> m_bottom;
};

} // namespace engine::threading
