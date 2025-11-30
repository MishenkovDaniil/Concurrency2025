#pragma once
#include <atomic>
#include <vector>
#include <cstddef>
#include <cassert>
#include <optional>
#include <functional>

template<typename T>
class ChaseLevDeque {
public:
    explicit ChaseLevDeque(size_t capacity)
        : cap(next_power_of_two(capacity)),
          mask(cap - 1),
          buf(cap),
          top(0),
          bottom(0)
    {
        for (size_t i = 0; i < cap; ++i)
            buf[i].store(nullptr, std::memory_order_relaxed);
    }

    bool push_bottom(T* item) {
        size_t b = bottom.load(std::memory_order_relaxed);
        size_t t = top.load(std::memory_order_acquire);
        if (b - t >= cap) {
            return false; // bug: should be resize
        }

        buf[b & mask].store(item, std::memory_order_relaxed);

        // publication of item
        bottom.store(b + 1, std::memory_order_release);
        return true;
    }

    T* pop_bottom() {
        size_t b = bottom.load(std::memory_order_relaxed);
        if (b == 0)
            return nullptr;

        b = b - 1;
        bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        size_t t = top.load(std::memory_order_acquire);
        if (t <= b) {
            T* item = buf[b & mask].load(std::memory_order_relaxed);
            if (t == b) {
                // sync with possible steal
                if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                    // smb stole it
                    item = nullptr;
                }
                // restore prev bottom if it was stolen
                bottom.store(b + 1, std::memory_order_relaxed);
            }
            return item;
        } else {
            // empty
            bottom.store(b + 1, std::memory_order_relaxed);
            return nullptr;
        }
    }

    T* steal_top() {
        size_t t = top.load(std::memory_order_acquire);
        size_t b = bottom.load(std::memory_order_acquire);
        if (t >= b)
            return nullptr; // empty

        T* item = buf[t & mask].load(std::memory_order_relaxed);
        if (top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
            return item;
        }
        return nullptr;
    }

    size_t get_size() const {
        // load top and bottom with acquire to see published changes
        size_t t = top.load(std::memory_order_acquire);
        size_t b = bottom.load(std::memory_order_acquire);
        if (b < t)
            return 0;
        return b - t;
    }

private:
    static size_t next_power_of_two(size_t x) {
        size_t n = 1;
        while (n < x)
            n <<= 1;
        return n;
    }

    const size_t cap;
    const size_t mask;
    std::vector<std::atomic<T*>> buf;
    std::atomic<size_t> top;
    std::atomic<size_t> bottom;
};
