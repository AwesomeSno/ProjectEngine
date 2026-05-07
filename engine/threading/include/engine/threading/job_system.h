// =============================================================================
// Project Engine - Job System API
// =============================================================================
// © Harinandan J V. All Rights Reserved.
// =============================================================================
// Core API for dispatching and waiting on jobs. 
// Features: Parent-Child relationships, Data-driven Continuations.
// =============================================================================

#pragma once

#include "engine/core/types.h"

namespace engine::threading {

struct JobCounter;

// The structure users pass when dispatching work.
struct JobDecl {
    // parent_counter is passed so the job can dispatch child jobs linked to the same counter
    void (*task)(void* data, u32 thread_index, JobCounter* parent_counter) = nullptr;
    void* data = nullptr;
};

namespace job_system {

    void init();
    void shutdown();
    [[nodiscard]] u32 get_worker_count();

    /// Allocate a job counter from the lock-free pool.
    /// Starts with count=1 to prevent early firing during dispatch.
    /// @param auto_free If true, the counter automatically frees itself when it hits 0.
    [[nodiscard]] JobCounter* allocate_counter(bool auto_free = false);

    /// Manually free a counter (only valid if auto_free was false).
    void free_counter(JobCounter* counter);

    /// Attach a continuation job. When `counter` reaches 0, the `continuation` job
    /// is automatically dispatched. `continuation_parent` will be passed as its counter.
    /// MUST be called before `release_counter()`.
    void set_continuation(JobCounter* counter, const JobDecl& continuation, JobCounter* continuation_parent = nullptr);

    /// Dispatch a single job attached to `counter`. Increments `counter` by 1.
    void dispatch(const JobDecl& job, JobCounter* counter = nullptr);

    /// Dispatch a batch of jobs attached to `counter`. Increments `counter` by `count`.
    void dispatch(u32 count, const JobDecl* jobs, JobCounter* counter = nullptr);

    /// Release the initial +1 hold on the counter.
    /// If it hits 0, it triggers the continuation and auto-frees (if enabled).
    void release_counter(JobCounter* counter);

    /// Block until the counter reaches 0. You MUST ensure auto_free is false!
    /// To prevent stalling, the waiting thread will execute available jobs.
    void wait(const JobCounter* counter);

    [[nodiscard]] bool is_busy(const JobCounter* counter);

} // namespace job_system
} // namespace engine::threading
