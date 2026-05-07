// =============================================================================
// Project Engine - ECS Event Streams
// =============================================================================

#pragma once

#include "engine/core/types.h"
#include "engine/core/assert.h"
#include <atomic>
#include <type_traits>
#include <vector>

namespace engine::ecs {

class IEventStream {
public:
    virtual ~IEventStream() = default;
    virtual void clear() = 0;
};

// High-performance, lock-free, append-only data stream for events.
// Enforces trivial copyability to ensure zero heap-allocation overhead or complex lifecycles during emission.
template<typename T, u32 Capacity = 8192>
class EventStream : public IEventStream {
    static_assert(std::is_trivially_copyable_v<T>, "Events must be trivially copyable PODs to guarantee fast, deterministic streams.");

public:
    void emit(const T& event) {
        u32 index = m_count.fetch_add(1, std::memory_order_relaxed);
        ENGINE_ASSERT_MSG(index < Capacity, "EventStream capacity exceeded.");
        if (index < Capacity) {
            m_data[index] = event;
        }
    }

    void clear() override {
        m_count.store(0, std::memory_order_relaxed);
    }

    const T* begin() const { return m_data; }
    const T* end() const { 
        u32 count = m_count.load(std::memory_order_relaxed);
        if (count > Capacity) count = Capacity;
        return m_data + count; 
    }
    
    u32 size() const {
        u32 count = m_count.load(std::memory_order_relaxed);
        return count > Capacity ? Capacity : count;
    }

private:
    std::atomic<u32> m_count{0};
    T m_data[Capacity];
};

// Central bus to hold all active event streams, enabling the Scheduler to automatically clear them at frame boundaries.
class EventBus {
public:
    ~EventBus() {
        for (IEventStream* stream : m_streams) {
            delete stream;
        }
    }

    template<typename T>
    EventStream<T>* register_stream() {
        EventStream<T>* stream = new EventStream<T>();
        m_streams.push_back(stream);
        return stream;
    }

    // Invoked by the Scheduler to flush all events after a topological layer finishes or at frame start
    void clear_all() {
        for (IEventStream* stream : m_streams) {
            stream->clear();
        }
    }

private:
    std::vector<IEventStream*> m_streams;
};

} // namespace engine::ecs
