<div align="center">

```
███████╗███╗   ██╗ ██████╗ ██╗███╗   ██╗███████╗
██╔════╝████╗  ██║██╔════╝ ██║████╗  ██║██╔════╝
█████╗  ██╔██╗ ██║██║  ███╗██║██╔██╗ ██║█████╗  
██╔══╝  ██║╚██╗██║██║   ██║██║██║╚██╗██║██╔══╝  
███████╗██║ ╚████║╚██████╔╝██║██║ ╚████║███████╗
╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝╚═╝  ╚═══╝╚══════╝
```

**A game engine. Built from nothing. For everything.**

[![Status](https://img.shields.io/badge/Status-In%20Development-blueviolet?style=flat-square)]()
[![Language](https://img.shields.io/badge/Language-C%2B%2B20%2F23-orange?style=flat-square)]()
[![Simulation](https://img.shields.io/badge/Simulation-120Hz%20Fixed%20Step-green?style=flat-square)]()
[![License](https://img.shields.io/badge/License-Proprietary-red?style=flat-square)]()

</div>

---

This repository contains a **small, intentional slice** of Project Engine — a proprietary, ground-up game engine and simulation platform currently under active development.

This is not the full project. This is a proof that it exists.

---

## What This Is

Project Engine is a complete game engine being built entirely from scratch in C++20/23. No Unity. No Unreal. No shortcuts.

The goal is a unified platform for interactive entertainment and high-fidelity simulation — with physically accurate rendering, musculoskeletal animation, dual-mode AI (classical + neural), and ray-traced spatial audio.

The architecture prioritises one thing above everything else: **performance that cannot be questioned.**

---

## What You Are Looking At

The files in this repository are a fragment of the engine's execution core.

```
engine/
├── core/
│   └── frame_contract.h        ← The 120Hz simulation contract
├── ecs/
│   ├── types.h                 ← Entity encoding (20-bit index, 12-bit generation)
│   ├── chunk.h                 ← 16KB cache-aligned SoA memory block
│   ├── archetype.h             ← Component layout manager
│   ├── registry.h              ← O(1) sparse-set entity indexing
│   ├── system_descriptor.h     ← System execution contract
│   └── query.h                 ← Chunk-to-Job dispatcher
└── threading/
    ├── work_stealing_queue.h   ← Lock-free Chase-Lev deque
    ├── fiber.h                 ← Stackful coroutine pool
    └── job_system.h            ← Fiber-based work-stealing job system
```

---

## Where The Engine Stands

### ✅ Completed — Execution Foundation

| Component | Detail |
|---|---|
| Frame Contract | Hybrid timestep: 120Hz fixed simulation, variable rendering, interpolated state |
| Scheduler | Kahn-sorted topological dependency resolver with full transitive data hazard detection |
| Job System | Lock-free Chase-Lev work-stealing deque, C++20 atomic wait/notify, 50k+ jobs/frame |
| Fiber Pool | 512 stackful fibers, yield/resume, zero-blocking wait on job counters |
| ECS Memory | 16KB cache-aligned Struct of Arrays chunks, O(1) sparse set entity lookup |
| Query Dispatcher | Iterates archetypes, slices into chunks, and saturates the thread pool in a single call |
| System Descriptor | Compile-time FNV-1a SystemID hashing, ComponentMask hazard detection, DataFreshness cascade |

### 🔄 In Progress

- Physics integration (Jolt Physics @ 120Hz)
- Rendering Hardware Interface (RHI)
- Archetype structural change (add/remove component paths)
- Frame Allocator (lock-free bump allocator, full implementation)

---

## The Execution Model

Every frame, the engine does this:

```
Frame ─────────────────────────────────────────────────────
│
├── [Simulation Phase — 120Hz Fixed]
│   ├── Layer 0: [Input System]         ─── dispatches N chunk jobs ─┐
│   ├── Layer 1: [Physics System]       ─── dispatches N chunk jobs ─┤ ← All run in
│   ├── Layer 2: [Animation System]     ─── dispatches N chunk jobs ─┤   parallel across
│   └── Layer 3: [AI System — 30Hz]     ─── dispatches N chunk jobs ─┘   all CPU cores
│
├── [Render Prep Phase — Variable]
│   └── Interpolation pass (alpha blending prev/current state)
│
└── [Render Phase — Variable]
    └── GPU submission
```

Each system:
1. Declares its `ComponentMask` (what data it reads and writes)
2. The scheduler detects conflicts automatically and orders execution
3. At runtime, the system calls `Query::dispatch()` once — which slices memory into 16KB chunks and fires a job per chunk into the lock-free thread pool

A single system processing 100,000 entities spawns hundreds of parallel jobs instantly.

---

## What Is Missing (By Design)

This repository does not contain:

- Build system (CMakeLists)
- Implementation files (.cpp)
- The full engine source
- Any documentation
- Anything that would let you compile or run this

---

## The Architecture (Briefly)

**Memory**: Entities are stored in 16KB cache-aligned Struct of Arrays chunks. No heap allocation at runtime. No pointer chasing between components.

**Threading**: A lock-free Chase-Lev work-stealing deque. Worker threads never spin or sleep — they steal chunks from each other's queues. When a fiber blocks on a dependency, it yields its stack and is immediately replaced by another runnable fiber.

**Scheduling**: Systems declare their data requirements at registration. A Kahn topological sort groups them into parallelizable layers. Transitive data hazards are detected via full closure DFS. Frequency mismatches (30Hz AI on a 120Hz loop) cascade correctly through freshness policies.

**Execution**: The `Query::dispatch()` call is the convergence point. One call triggers the entire multi-core execution path.

---

<div align="center">

**© AxeomLabs. All rights reserved.**

*This project is proprietary and not licensed for external use.*

*Nothing here is enough to build anything. That is intentional.*

</div>
