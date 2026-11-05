// SPSC (Single-Producer Single-Consumer) bounded queue — phiên bản v1.
//
// v1 = ĐÚNG nhưng CHƯA TỐI ƯU. Nhiệm vụ của bạn là tạo v2, v3 (xem README.md) và đo
// mức cải thiện ở mỗi bước.
//
// Bất biến:
//   - Chỉ producer ghi tail_, chỉ consumer ghi head_.
//   - Queue rỗng khi head_ == tail_; đầy khi (tail_ + 1) % size == head_
//     (bỏ trống 1 slot để phân biệt rỗng/đầy).
//
// Memory ordering (cặp release/acquire):
//   producer: ghi buf_[t]  ──►  tail_.store(release)
//   consumer: tail_.load(acquire)  ──►  đọc buf_[h]
//   => Consumer thấy tail_ mới thì chắc chắn thấy dữ liệu đã ghi vào slot.
//   Ngược lại cho head_: consumer báo "đã đọc xong slot", producer mới được ghi đè.
#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

template <class T>
class SpscQueueV1 {
public:
    explicit SpscQueueV1(std::size_t capacity) : buf_(capacity + 1) {}

    SpscQueueV1(const SpscQueueV1&) = delete;
    SpscQueueV1& operator=(const SpscQueueV1&) = delete;

    // Chỉ gọi từ thread producer.
    bool try_push(const T& value) noexcept {
        const std::size_t t = tail_.load(std::memory_order_relaxed);  // chỉ mình ta ghi tail_
        std::size_t next = t + 1;
        if (next == buf_.size()) next = 0;  // phép % chậm hơn so sánh (bài tập: dùng mask 2^k)
        if (next == head_.load(std::memory_order_acquire)) return false;  // đầy
        buf_[t] = value;
        tail_.store(next, std::memory_order_release);  // "công bố" slot t
        return true;
    }

    // Chỉ gọi từ thread consumer.
    bool try_pop(T& out) noexcept {
        const std::size_t h = head_.load(std::memory_order_relaxed);  // chỉ mình ta ghi head_
        if (h == tail_.load(std::memory_order_acquire)) return false;  // rỗng
        out = buf_[h];
        std::size_t next = h + 1;
        if (next == buf_.size()) next = 0;
        head_.store(next, std::memory_order_release);  // trả slot h cho producer
        return true;
    }

    std::size_t capacity() const noexcept { return buf_.size() - 1; }

private:
    std::vector<T> buf_;
    // VẤN ĐỀ CỦA v1 (để bạn sửa ở v2/v3):
    //  (1) head_ và tail_ nằm chung cache line -> false sharing giữa producer và consumer.
    //  (2) Mỗi lần push đều đọc head_ (line do consumer sở hữu) và mỗi lần pop đều đọc tail_
    //      -> cache line liên tục bị chuyển qua lại giữa 2 core.
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};
