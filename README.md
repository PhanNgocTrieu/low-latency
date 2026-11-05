# Low Latency Systems — Lộ trình tự học

Mục tiêu: trong khoảng **6 tháng (10–15 giờ/tuần)**, đi từ nền tảng đến mức có thể
phỏng vấn cho vị trí **Low Latency C++ / Trading Systems Engineer** (kể cả remote),
với một **portfolio GitHub** gồm các project có số liệu benchmark thật.

> Nguyên tắc xuyên suốt: **"Đo trước, tối ưu sau"** (measure, don't guess).
> Mỗi project đều phải có số liệu: p50 / p99 / p99.9 / max, không chỉ là trung bình.

---

## Cấu trúc repo

```
README.md                 ← bạn đang đọc (lộ trình + checklist tiến độ)
Makefile                  ← build tất cả project:  make  /  make run-01 ...
docs/
  01-cpp-performance.md        C++ hiện đại cho hiệu năng
  02-computer-architecture.md  Cache, branch prediction, TLB, NUMA, pipeline
  03-concurrency-lockfree.md   Atomics, memory ordering, lock-free queues
  04-linux-networking.md       Tuning Linux, kernel bypass, network stack
  05-measurement.md            Đo latency đúng cách, perf, histogram
  06-trading-domain.md         Kiến thức thị trường: order book, ITCH, FIX
  07-interview-job-search.md   Câu hỏi phỏng vấn + chiến lược tìm việc remote
  resources.md                 Sách, talk, blog, code nên đọc
projects/
  common/                      Header dùng chung: timer, thống kê percentile
  01-hardware-effects/   ✅ chạy được  — đo cache, branch, false sharing
  02-spsc-queue/         ✅ chạy được  — lock-free ring buffer + bench
  03-order-book/         ✅ baseline   — matching engine, bạn tối ưu tiếp
  04-memory-pool/        📝 spec       — pool allocator, intrusive list
  05-feed-handler/       📝 spec       — parse NASDAQ ITCH 5.0 thật
  06-async-logger/       📝 spec       — logger nanosecond-level
  07-network-latency/    📝 spec       — UDP/TCP, busy-poll, io_uring (Linux)
  08-capstone-tick-to-trade/ 📝 spec   — hệ thống end-to-end
```

Build nhanh (macOS hoặc Linux, cần clang/gcc hỗ trợ C++20):

```bash
make            # build tất cả
make run-01     # chạy thí nghiệm phần cứng
make test-02    # test SPSC queue
make run-02     # benchmark SPSC queue
make test-03 run-03
```

### ⚠️ Lưu ý về môi trường (bạn đang dùng Mac M1 Pro)

- Mac phù hợp cho giai đoạn 1–3 (C++, cache, lock-free, order book).
- **Từ giai đoạn 4 bạn cần một máy Linux x86-64** — gần như 100% hệ thống HFT chạy
  Linux trên Intel/AMD. Trên macOS bạn **không** có: `perf`, `isolcpus`, thread pinning
  thật sự, `rdtsc`, kernel bypass, `io_uring`.
  Lựa chọn rẻ: mini PC Intel/AMD cũ (~150–250 USD) cài Ubuntu, hoặc thuê dedicated
  server (Hetzner/OVH) theo tháng. VM/cloud thông thường đo jitter không chuẩn, nhưng
  vẫn dùng được để học `perf` và tuning.
- Bộ đếm thời gian trên Apple Silicon có độ phân giải ~41.7 ns (24 MHz). Muốn đo thao tác
  nhỏ hơn mức đó → đo theo **batch** rồi chia trung bình.

---

## Lộ trình 6 tháng

| Giai đoạn | Tuần | Chủ đề | Đọc | Làm |
|---|---|---|---|---|
| 0. Khởi động | 1–2 | Đo lường cơ bản, latency numbers | docs/05, docs/02 (phần 1) | Project 01 |
| 1. Phần cứng & C++ | 3–6 | Cache, branch, data-oriented design, C++ zero-cost | docs/01, docs/02 | Mở rộng Project 01, đọc assembly trên godbolt |
| 2. Concurrency | 7–10 | Atomics, memory model, lock-free | docs/03 | Project 02, Project 06 |
| 3. Trading domain | 11–14 | Order book, matching, market data | docs/06 | Project 03, Project 04 |
| 4. Linux & mạng | 15–18 | Tuning OS, perf, network stack, kernel bypass | docs/04 | Project 05, Project 07 (trên Linux) |
| 5. Capstone & tìm việc | 19–24 | Hệ thống end-to-end, viết blog, phỏng vấn | docs/07 | Project 08, mock interview |

Nhịp mỗi tuần gợi ý:
- 40% đọc/xem talk → ghi chú vào `notes/` (tự tạo) bằng lời của chính bạn.
- 50% code + benchmark.
- 10% viết: mỗi project xong → 1 bài blog / README có biểu đồ latency. Đây là thứ
  nhà tuyển dụng remote thực sự đọc.

---

## Checklist tiến độ

### Nền tảng
- [ ] Thuộc bảng "Latency numbers every programmer should know" và giải thích được vì sao
- [ ] Giải thích cache line, false sharing, prefetcher, TLB miss
- [ ] Đọc được assembly x86-64 / ARM64 cơ bản trên godbolt.org
- [ ] Hiểu `memory_order_acquire/release/seq_cst` và vẽ được happens-before

### Projects
- [ ] 01 — Vẽ được biểu đồ latency theo kích thước working set (thấy rõ L1/L2/L3/DRAM)
- [ ] 02 — SPSC queue: tối ưu từ v1 → v3, ghi lại throughput + RTT từng bước
- [ ] 03 — Order book: từ baseline `std::map` → cấu trúc phẳng, p99 giảm ≥ 5x
- [ ] 04 — Pool allocator, áp dụng vào order book, 0 malloc trên hot path
- [ ] 05 — Feed handler ITCH: xử lý file thật cả ngày, đo msgs/giây
- [ ] 06 — Async logger: hot path < 50 ns p99
- [ ] 07 — Đo RTT UDP qua loopback / 2 máy; so sánh blocking vs busy-poll
- [ ] 08 — Capstone tick-to-trade có histogram end-to-end + bài viết

### Tìm việc
- [ ] GitHub sạch, mỗi repo có README + số liệu + cách tái tạo
- [ ] 2–3 bài blog kỹ thuật (LinkedIn / Medium / blog cá nhân)
- [ ] Luyện 50 câu hỏi trong docs/07
- [ ] Gửi 5–10 hồ sơ/tuần từ tháng thứ 5
