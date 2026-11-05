# Module 02 — Kiến trúc máy tính cho lập trình viên low latency

**Mục tiêu:** dự đoán được một đoạn code nhanh hay chậm *trước khi* đo, rồi dùng số liệu
để kiểm chứng. Đây là phần phỏng vấn HFT hỏi nhiều nhất.

## 1. Latency numbers (cỡ độ lớn, CPU ~3–5 GHz)

| Thao tác | Xấp xỉ |
|---|---|
| 1 chu kỳ CPU | ~0.3 ns |
| L1 hit | ~1 ns (4–5 cycles) |
| Branch mispredict | ~5 ns (15–20 cycles) |
| L2 hit | ~3–4 ns |
| L3 hit | ~10–20 ns |
| Atomic RMW có tranh chấp giữa core | ~20–100 ns |
| DRAM (cache miss) | ~70–100 ns |
| Syscall / context switch | ~100 ns – vài µs |
| UDP qua kernel stack (loopback) | ~2–10 µs |
| Kernel bypass NIC one-way | ~1 µs hoặc thấp hơn |
| FPGA tick-to-trade | vài chục – vài trăm ns |
| RTT trong cùng datacenter | ~10–50 µs |

Project 01 cho bạn tự đo các con số này.

## 2. Cache hierarchy
- **Cache line**: 64 byte trên x86; Apple M-series báo 128 byte. Đọc 1 byte = tải cả line.
- **Spatial / temporal locality**.
- **Hardware prefetcher** đoán truy cập tuần tự/stride. Pointer chasing (list, tree) phá prefetcher.
- **Associativity & conflict miss**: stride lũy thừa 2 (vd 4096 byte) dồn vào cùng set.
- **Coherence (MESI/MOESI)**: 2 core ghi cùng 1 line → line "ping-pong" → **false sharing**.
  Giải pháp: `alignas(64)` / `alignas(128)`.
- **Store buffer**: store không chặn ngay nhưng có giới hạn dung lượng.

## 3. Branch prediction & pipeline
- Đoán sai → xả pipeline (~15–20 cycles). Dữ liệu ngẫu nhiên + rẽ nhánh ≈ sai 50%.
- Kỹ thuật: sắp xếp dữ liệu, branchless (`cmov`/`csel`), lookup table, `[[likely]]`.
- Đọc thêm: out-of-order execution, BTB, return stack buffer.

## 4. Virtual memory & TLB
- TLB miss → page walk. **Huge pages** (2 MB/1 GB) giảm TLB miss.
- **Page fault** lần đầu chạm trang rất đắt (µs) → `mlockall()` + pre-fault lúc khởi động.

## 5. NUMA
- Truy cập RAM của socket khác chậm ~1.5–2x. Dùng `numactl --cpunodebind=0 --membind=0`.
- NIC gắn với 1 socket → thread mạng đặt cùng socket.

## 6. SIMD
- SSE/AVX2/AVX-512 (x86), NEON (ARM). Dùng cho parse, tìm kiếm, checksum.
- AVX-512 trên một số Intel cũ gây giảm xung nhịp → ảnh hưởng latency.

## 7. Bài tập (gắn với Project 01)
1. `make run-01`, vẽ ns/hop theo kích thước working set (log scale). Đánh dấu L1/L2/L3/DRAM.
2. Thêm: duyệt mảng 2D theo hàng vs cột (1024x1024 `int`).
3. Thêm: stride access với stride 1, 2, 4, …, 4096.
4. Trên Linux: `perf stat -e cache-misses,branch-misses,dTLB-load-misses` cho từng thí nghiệm.

## Đọc / xem
- Ulrich Drepper — *What Every Programmer Should Know About Memory* (mục 2, 3, 6) ⭐
- Bryant & O'Hallaron — *Computer Systems: A Programmer's Perspective*, ch. 5, 6, 9 ⭐
- Denis Bakhvalov — *Performance Analysis and Tuning on Modern CPUs* (miễn phí)
- Timur Doumler — *Want fast C++? Know your hardware!* (CppCon 2016)
- Agner Fog — *The microarchitecture of Intel, AMD and VIA CPUs*
