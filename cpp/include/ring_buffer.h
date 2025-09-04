#pragma once
#include <vector>
#include <atomic>
#include <cstddef>
#include <optional>

// Lock-free single-producer single-consumer ring buffer.
namespace acq {

template <class T>
class SPSCRingBuffer {
public:
    explicit SPSCRingBuffer(size_t capacity_pow2)
        : capacity_(capacity_pow2), mask_(capacity_pow2 - 1), data_(capacity_pow2) {
        // capacity must be power of two
    }

    bool push(const T& v) {
        auto head = head_.load(std::memory_order_relaxed);
        auto next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false; // full
        }
        data_[head] = v;
        head_.store(next, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt; // empty
        }
        T v = data_[tail];
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return v;
    }

    bool empty() const { return head_.load() == tail_.load(); }

private:
    size_t capacity_;
    size_t mask_;
    std::vector<T> data_;
    std::atomic<size_t> head_ {0};
    std::atomic<size_t> tail_ {0};
};

} // namespace acq
