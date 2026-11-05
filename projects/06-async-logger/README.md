# Project 06 — Async low latency logger

**Đọc kèm:** docs/03; tham khảo `odygrd/quill`, `PlatformLab/NanoLog`

## Vì sao
`printf`/`std::cout`/spdlog đồng bộ: format chuỗi (hàng trăm ns) + syscall `write` (µs) +
có thể lock. Hot path không được phép tốn chi phí đó, nhưng vẫn cần log để debug/audit.

## Thiết kế
```
Hot thread:  LOG("order {} filled {} @ {}", id, qty, px)
   → ghi (format_id, timestamp_ticks, args nhị phân) vào SPSC ring buffer của thread đó
Background thread:
   → đọc ring buffer, format chuỗi, ghi file theo batch
```
- **Deferred formatting**: hot path chỉ `memcpy` tham số; format ở thread nền.
- Format string được đăng ký 1 lần (static) → hot path chỉ ghi id.
- Timestamp bằng `ticks()` rẻ, đổi ra thời gian thật ở thread nền.
- Mỗi thread hot có queue riêng (thread_local) → không tranh chấp.
- Chính sách khi đầy: drop + đếm số log bị mất (không bao giờ block hot path).

## Yêu cầu
1. API variadic template, kiểm tra kiểu lúc compile.
2. Hỗ trợ int, double, `const char*` (string literal), `std::string_view` (copy nội dung).
3. Benchmark: latency của 1 lời gọi LOG trên hot thread — mục tiêu p99 < 50 ns
   (trên Linux x86 tuned). So sánh với `fprintf` và spdlog (sync và async).
4. Test: log 10 triệu dòng từ 4 thread, xác minh không mất/không hỏng dòng nào.
