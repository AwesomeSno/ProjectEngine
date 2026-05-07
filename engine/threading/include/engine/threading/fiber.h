// =============================================================================
// Project Engine - Fiber API
// =============================================================================
// © Harinandan J V. All Rights Reserved.
// =============================================================================
// Cross-platform stackful coroutines (Fibers).
// Abstracts Windows Fibers and POSIX ucontext for preemptive job suspension.
// =============================================================================

#pragma once
#include "engine/core/types.h"

namespace engine::threading {

typedef void* FiberHandle;
typedef void (*FiberEntryFunc)(void* data);

/// Creates a new fiber with the specified stack size (e.g., 512KB).
FiberHandle fiber_create(usize stack_size, FiberEntryFunc entry, void* data);

/// Destroys a created fiber. Do not call on the thread fiber.
void fiber_destroy(FiberHandle fiber);

/// Converts the current OS thread into a fiber. Returns its handle.
FiberHandle fiber_convert_thread();

/// Suspends the current fiber and resumes the target fiber.
void fiber_switch(FiberHandle target);

/// Returns the handle of the currently executing fiber.
FiberHandle fiber_current();

} // namespace engine::threading
