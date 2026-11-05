# Module 05 — Đo latency đúng cách

**Mục tiêu:** số liệu của bạn đáng tin. Trong phỏng vấn, người ta hỏi "bạn đo bằng cách
nào?" nhiều hơn là "nhanh bao nhiêu?".

## 1. Nguyên tắc
1. **Latency là một phân phối, không phải một con số.** Báo cáo min, p50, p90, p99, p99.9,
   p99.99, max. Trong trading, *tail latency* (p99+) quan trọng hơn trung bình.
2. **Không dùng trung bình để kết luận.** Một spike 1 ms che giấu trong trung bình nhưng có
   thể làm mất cả giao dịch.
3. **Coordinated omission** (Gil Tene): nếu bộ sinh tải chờ hệ thống trả lời rồi mới gửi tiếp,
   bạn bỏ sót đúng những lúc hệ thống chậm. Dùng tải với lịch gửi cố định và đo từ thời
   điểm *dự định* gửi.
4. **Warm-up**: bỏ các lần chạy đầu (cache lạnh, page fault, branch predictor chưa học).
5. **Chống compiler xóa code benchmark**: `DoNotOptimize`, `ClobberMemory`
   (xem `projects/common/bench.hpp`).
6. **Môi trường ổn định**: cố định tần số CPU, pin thread, tắt ứng dụng khác, chạy nhiều lần.
7. **Chi phí của chính bộ đo**: đo `now()` liên tiếp 2 lần để biết overhead (~20 ns với
   `clock_gettime` vDSO, ~7–10 ns với `rdtsc` trên x86).

## 2. Đồng hồ
| Nguồn | Nền tảng | Ghi chú |
|---|---|---|
| `std::chrono::steady_clock` | Mọi nơi | Đơn giản, đủ cho đa số trường hợp |
| `clock_gettime(CLOCK_MONOTONIC)` | Linux | Qua vDSO, không syscall |
| `rdtsc` / `rdtscp` | x86 | Rất rẻ; cần `constant_tsc`, `nonstop_tsc`; phải hiệu chuẩn sang ns |
| `cntvct_el0` | ARM64 | Apple: 24 MHz → độ phân giải ~41.7 ns |
| NIC hardware timestamp | Linux + NIC hỗ trợ | Đo "on the wire", chuẩn nhất cho tick-to-trade |

## 3. Công cụ
- **Google Benchmark** / **nanobench**: micro-benchmark có thống kê.
- **HdrHistogram**: ghi lại phân phối latency với bộ nhớ cố định — cần cho hệ thống chạy lâu.
- **perf** (Linux): `perf stat -d`, `perf record -g`, flame graph.
- **Intel VTune**, **AMD uProf**, **pmu-tools/toplev** (Top-down Microarchitecture Analysis).
- macOS: **Instruments** (Time Profiler, Counters) — đủ để học giai đoạn đầu.
- **godbolt.org**: xem assembly.
- `uftrace`, `ltrace`, `strace -c` (đếm syscall — hot path phải là 0).

## 4. Quy trình tối ưu
```
Đặt mục tiêu (vd p99 < 2 µs) → Đo baseline → Profile tìm bottleneck
 → Đặt giả thuyết (vd "cache miss ở lookup order") → Sửa 1 thứ → Đo lại → Ghi nhật ký
```
Ghi **nhật ký tối ưu** (bảng: thay đổi, p50, p99, max, giải thích). Đây chính là nội dung
README/blog cho portfolio của bạn.

## 5. Bài tập
1. Đo overhead của `steady_clock::now()` trên Mac của bạn (gọi liên tiếp 1 triệu lần).
2. Tự viết HDR-style histogram: bucket theo lũy thừa 2 + sub-bucket tuyến tính; so sánh kết
   quả percentile với cách sort toàn bộ mẫu.
3. Viết load generator gửi theo lịch cố định (vd 100k msg/s) để tránh coordinated omission.

## Đọc / xem
- Gil Tene — *How NOT to Measure Latency* ⭐
- Brendan Gregg — *Systems Performance*, ch. 2 (Methodologies)
- Denis Bakhvalov — *Performance Analysis and Tuning on Modern CPUs*, phần đo lường ⭐
- Tài liệu Google Benchmark: phần "Preventing optimization"
- Intel — *How to Benchmark Code Execution Times on Intel IA-32 and IA-64* (whitepaper về rdtsc)
