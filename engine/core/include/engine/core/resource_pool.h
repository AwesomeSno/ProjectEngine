// =============================================================================
// Project Engine - Resource Pool
// =============================================================================

#pragma once

#include "engine/core/handle.h"
#include "engine/core/assert.h"
#include <vector>
#include <new>

namespace engine::core {

// A high-performance, ABA-safe contiguous resource allocator.
// Enforces generation tracking to instantly detect and reject stale handles.
template<typename T>
class ResourcePool {
public:
    explicit ResourcePool(u32 capacity = 8192) : m_capacity(capacity) {
        m_data.resize(capacity);
        m_generations.resize(capacity, 1); // Generation starts at 1 (0 is invalid)
        m_free_indices.resize(capacity);

        // Intrusive linked list setup for O(1) free/alloc without reallocations
        for (u32 i = 0; i < capacity; ++i) {
            m_free_indices[i] = i + 1; // Point to next free slot
        }
        m_free_head = 0;
        m_active_count = 0;
    }

    Handle<T> allocate() {
        ENGINE_ASSERT_MSG(m_free_head < m_capacity, "ResourcePool capacity exceeded. Out of handles.");
        
        u32 index = m_free_head;
        m_free_head = m_free_indices[index];
        m_active_count++;

        // Placement new to construct the object cleanly over the recycled memory
        new (&m_data[index]) T();

        return Handle<T>{index, m_generations[index]};
    }

    Handle<T> allocate(const T& value) {
        Handle<T> h = allocate();
        m_data[h.index] = value;
        return h;
    }

    void free(Handle<T> handle) {
        if (!is_valid(handle)) return;

        // Explicitly destruct to release external memory
        m_data[handle.index].~T();

        // Increment generation to instantly invalidate all stale handles
        m_generations[handle.index]++;
        
        // Push back onto the intrusive free list
        m_free_indices[handle.index] = m_free_head;
        m_free_head = handle.index;
        
        m_active_count--;
    }

    bool is_valid(Handle<T> handle) const {
        if (!handle.is_valid()) return false;
        if (handle.index >= m_capacity) return false;
        return m_generations[handle.index] == handle.generation;
    }

    T* get(Handle<T> handle) {
        if (!is_valid(handle)) return nullptr;
        return &m_data[handle.index];
    }

    const T* get(Handle<T> handle) const {
        if (!is_valid(handle)) return nullptr;
        return &m_data[handle.index];
    }

    u32 active_count() const {
        return m_active_count;
    }

    u32 capacity() const {
        return m_capacity;
    }

private:
    u32 m_capacity;
    u32 m_free_head;
    u32 m_active_count;
    
    std::vector<T> m_data;
    std::vector<u32> m_generations;
    std::vector<u32> m_free_indices;
};

} // namespace engine::core
