# Module 03 — Concurrency & Lock-free

**Mục tiêu:** hiểu memory model C++ đủ sâu để viết và *chứng minh đúng* một hàng đợi
lock-free; biết khi nào lock-free không đáng.

## 1. Kiến trúc thread điển hình của hệ thống trading

```
 [NIC] → Thread market data (busy-spin, pin core 2)
            │ SPSC queue
            ▼
         Thread strategy (busy-spin, pin core 3)
            │ SPSC queue
            ▼
         Thread order gateway (pin core 4) → [NIC]
         Thread logging / monitoring (core không cách ly, ưu tiên thấp)
```

- Mỗi thread hot path **độc quyền 1 core** (`isolcpus`, `taskset`, `pthread_setaffinity_np`).
- **Busy-spin** thay vì sleep/condition variable: đánh đổi 100% CPU lấy latency thấp.
- Giao tiếp giữa thread bằng **SPSC queue** — đơn giản nhất, nhanh nhất.
- Thường tránh hẳn chia sẻ trạng thái: "share nothing, pass messages".

## 2. Memory model C++

| Ordering | Ý nghĩa | Dùng khi |
|---|---|---|
| `relaxed` | Chỉ đảm bảo atomic, không thứ tự | Counter thống kê |
| `acquire` (load) | Lệnh sau không bị đưa lên trước load | Đọc cờ "dữ liệu đã sẵn sàng" |
| `release` (store) | Lệnh trước không bị đưa xuống sau store | Ghi cờ "dữ liệu đã sẵn sàng" |
| `acq_rel` | Cả hai (cho RMW) | CAS trong cấu trúc lock-free |
| `seq_cst` | Thứ tự toàn cục duy nhất | Mặc định; đắt hơn trên một số kiến trúc |

Cặp **release-store → acquire-load** đọc thấy giá trị đó tạo quan hệ *synchronizes-with*:
mọi thứ ghi trước release đều nhìn thấy sau acquire. Đây là toàn bộ nền tảng của SPSC queue.

x86 là TSO (mạnh): acquire/release gần như miễn phí, chỉ `seq_cst` store mới tốn
(`xchg`/`mfence`). ARM yếu hơn: dùng `ldar`/`stlr`. **Code sai memory order có thể chạy
đúng trên x86 nhưng hỏng trên ARM** — Mac M1 của bạn là môi trường test tốt cho việc này.

## 3. Các khái niệm phải giải thích được
- Lock-free vs wait-free vs obstruction-free.
- CAS loop, ABA problem (và cách xử lý: tagged pointer, hazard pointer, epoch-based reclamation).
- False sharing giữa head/tail của queue.
- Vì sao mutex chậm: không phải do lệnh lock, mà do **tranh chấp** + có thể vào kernel (futex) + context switch.
- Priority inversion, convoying.
- `std::atomic<T>::is_always_lock_free`.
- Memory reclamation là phần khó nhất của cấu trúc lock-free dạng node.

## 4. Các hàng đợi nên biết

| Loại | Ví dụ | Ghi chú |
|---|---|---|
| SPSC bounded ring | rigtorp/SPSCQueue, folly ProducerConsumerQueue | Project 02 |
| MPMC bounded | Dmitry Vyukov bounded MPMC, rigtorp/MPMCQueue | Mỗi slot có sequence number |
| Disruptor | LMAX Disruptor | Ring buffer + sequence barrier, multicast cho nhiều consumer |
| Seqlock | Linux kernel | 1 writer nhiều reader, reader retry — dùng cho snapshot giá |

## 5. Bài tập
1. Project 02 (SPSC) — từ v1 đến v3.
2. Viết **seqlock** cho struct `Quote { bid, ask, bid_qty, ask_qty }`; 1 writer, 3 reader;
   kiểm tra reader không bao giờ đọc được trạng thái "rách" (torn read).
3. Implement Vyukov bounded MPMC queue, so sánh với `std::mutex + std::deque` ở 1/2/4/8 thread.
4. Dùng ThreadSanitizer (`-fsanitize=thread`) chạy test các queue của bạn.

## Đọc / xem
- Anthony Williams — *C++ Concurrency in Action* (2nd ed.), ch. 5 & 7 ⭐
- Herb Sutter — *atomic<> Weapons* (C++ and Beyond 2012, 2 phần) ⭐
- Fedor Pikus — *The speed of concurrency (is lock-free faster?)* (CppCon 2016)
- Martin Thompson et al. — *LMAX Disruptor* technical paper
- Paul McKenney — *Is Parallel Programming Hard, And, If So, What Can You Do About It?* (miễn phí)
- Erik Rigtorp — mã nguồn SPSCQueue/MPMCQueue và blog rigtorp.se
- Dmitry Vyukov — 1024cores.net
- Herlihy & Shavit — *The Art of Multiprocessor Programming* (học thuật hơn)
