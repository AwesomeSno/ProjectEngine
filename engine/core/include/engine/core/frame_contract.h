// =============================================================================
// Project Engine - Frame Execution Contract
// =============================================================================
// © Harinandan J V. All Rights Reserved.
// =============================================================================
// This header enforces the deterministic hybrid-timestep frame model.
// It is not just documentation; it provides the compile-time guarantees,
// constants, and access boundaries required to prevent the engine from
// devolving into race conditions or non-deterministic behavior.
// =============================================================================

#pragma once

#include "engine/core/types.h"
#include "engine/core/compiler.h"
#include "engine/core/assert.h"

namespace engine::core {

// ---- Time Domains ----------------------------------------------------------

struct FrameTime {
    f64 wall_time;        // Raw OS time (do not use for simulation)
    f64 game_time;        // Scaled game time
    f32 delta_time;       // Variable step (dt)
    f32 fixed_delta_time; // Fixed step (fdt)
    f32 alpha;            // Interpolation factor [0.0, 1.0]
};

// ---- Strict Execution Constants --------------------------------------------

// The engine simulates at a strict 120Hz to maintain deterministic physics
// (stable soft-bodies, vehicle dynamics, fluid boundaries).
inline constexpr f32 k_TargetSimulationRate = 120.0f;
inline constexpr f32 k_FixedDeltaTime = 1.0f / k_TargetSimulationRate;

// Spiral of Death Protection:
// If the variable render frame takes longer than 5 fixed ticks (e.g., >41ms),
// we clamp the accumulator. We drop simulation frames to prevent the CPU
// from death-spiraling trying to catch up.
inline constexpr u32 k_MaxSimulationTicksPerFrame = 5;

// ---- State Aliasing (Pointer Swap / SoA Flip) ------------------------------

// Rather than doing a massive memcpy of 10,000 transforms, the engine
// uses a flip-flop index for double buffering spatial data.
enum class StateBuffer : u8 {
    BufferA = 0,
    BufferB = 1
};

inline StateBuffer flip_buffer(StateBuffer current) {
    return static_cast<StateBuffer>(1 - static_cast<u8>(current));
}

// ---- Ownership Enforcements ------------------------------------------------

// To be used by the ECS and Job System to strictly enforce read/write bounds.
// The Render phase MUST be ReadOnly. The Simulation phase MUST be WriteOnly (to the backbuffer).
template <typename T>
struct ReadOnly {
    const T* data;
    [[nodiscard]] const T& get() const { return *data; }
    [[nodiscard]] const T* operator->() const { return data; }
};

template <typename T>
struct WriteOnly {
    T* data;
    void set(const T& val) { *data = val; }
    // Intentionally no read access.
};

// ---- Determinism Constraints -----------------------------------------------

// All systems participating in the fixed tick MUST rely strictly on `k_FixedDeltaTime`
// and deterministic math functions. Floating point modes must be strictly controlled
// (e.g., IEEE 754 compliance, no fast-math across deterministic boundaries).
// System execution order must be topologically sorted at compile time.

} // namespace engine::core
