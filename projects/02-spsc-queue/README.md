# Project 02 — SPSC lock-free queue

**Đọc kèm:** [docs/03-concurrency-lockfree.md](../../docs/03-concurrency-lockfree.md)

SPSC queue là "mạch máu" của hệ thống low latency: nối thread market data → strategy →
order gateway. Câu hỏi "viết SPSC ring buffer" xuất hiện trong rất nhiều buổi phỏng vấn.

```bash
make test-02    # tính đúng đắn
make tsan-02    # chạy test dưới ThreadSanitizer
make run-02     # benchmark throughput + RTT
```

## Có sẵn
- `spsc_queue.hpp` — **v1**: đúng, dùng acquire/release, nhưng chưa tối ưu.
- `mutex_queue.hpp` — baseline dùng `std::mutex`.
- `test.cpp`, `bench.cpp`.

## Lộ trình tối ưu (mỗi bước: viết class mới, thêm vào test + bench, ghi số liệu)

| Phiên bản | Thay đổi | Câu hỏi bạn phải trả lời |
|---|---|---|
| v1 | (có sẵn) | Vì sao chỉ cần acquire/release, không cần seq_cst? |
| v2 | `alignas(kCacheLine)` cho `head_` và `tail_`; dung lượng làm tròn lên 2^k và dùng `& mask` thay vì so sánh/`%` | Cải thiện bao nhiêu? Vì sao? |
| v3 | **Cached index**: producer giữ bản sao `head_cache_` (chỉ đọc lại `head_` thật khi tưởng là đầy); consumer giữ `tail_cache_` tương tự | Giảm được bao nhiêu lần cache line di chuyển giữa 2 core? |
| v4 | Thêm padding trước/sau buffer để tránh false sharing với dữ liệu xung quanh; buffer là mảng cấp phát với alignment | |
| v5 | API zero-copy: `T* front()` / `pop()` và `emplace(args...)` thay vì copy; hỗ trợ T không default-constructible | |
| v6 (tùy chọn) | Batch: consumer lấy nhiều phần tử 1 lần, chỉ cập nhật `head_` 1 lần | Trade-off throughput vs latency? |

Tham khảo sau khi đã tự làm: `rigtorp/SPSCQueue` (gần với v3–v5), folly `ProducerConsumerQueue`.

## Nhật ký kết quả (điền vào)

Máy: ______ · OS: ______ · Compiler/flags: ______

| Phiên bản | Throughput (M ops/s) | RTT p50 (ns) | RTT p99 (ns) | RTT p99.9 (ns) | Ghi chú |
|---|---|---|---|---|---|
| MutexQueue | | | | | |
| v1 | | | | | |
| *tham khảo M1 Pro: Mutex* | 26.3 | 16792 | 74292 | 133459 | macOS, không pin core |
| *tham khảo M1 Pro: v1* | 29.1 | 167 | 792 | 1333 | throughput v1 chưa hơn mutex bao nhiêu → v2/v3 sẽ sửa |
| v2 | | | | | |
| v3 | | | | | |

## Lưu ý trên macOS
- Không pin được thread vào core → kết quả dao động; chạy nhiều lần lấy median.
- M1 có P-core và E-core: OS có thể xếp thread vào E-core → số liệu tệ bất thường.
- Chạy lại trên Linux x86 với `taskset`/`isolcpus` ở giai đoạn 4 và so sánh.

## Mở rộng
- Viết **seqlock** và **Vyukov bounded MPMC queue** (xem docs/03).
- Thử ARM vs x86: cố ý dùng `relaxed` cho `tail_.store` → test có fail trên M1 không? (Có thể
  cần chạy nhiều lần; đây là lý do lỗi memory ordering rất khó bắt.)
