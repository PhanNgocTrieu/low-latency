// Baseline: ring buffer bảo vệ bằng std::mutex. Dùng để so sánh với bản lock-free.
#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

template <class T>
class MutexQueue {
public:
    explicit MutexQueue(std::size_t capacity) : buf_(capacity + 1) {}

    bool try_push(const T& value) {
        std::lock_guard<std::mutex> lk(m_);
        std::size_t next = (tail_ + 1) % buf_.size();
        if (next == head_) return false;
        buf_[tail_] = value;
        tail_ = next;
        return true;
    }

    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lk(m_);
        if (head_ == tail_) return false;
        out = buf_[head_];
        head_ = (head_ + 1) % buf_.size();
        return true;
    }

    std::size_t capacity() const noexcept { return buf_.size() - 1; }

private:
    std::mutex m_;
    std::vector<T> buf_;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
};
