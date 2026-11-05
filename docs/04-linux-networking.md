# Module 04 — Linux tuning & Networking

**Mục tiêu:** biết biến một máy Linux "thường" thành máy có jitter thấp; hiểu đường đi
của một gói tin từ dây mạng đến ứng dụng và cách đi tắt (kernel bypass).

> Cần máy Linux x86-64 thật (xem README). Ghi lại **trước/sau** mỗi thay đổi bằng số liệu.

## 1. Nguồn gây jitter và cách xử lý

| Nguồn jitter | Biện pháp |
|---|---|
| Scheduler chuyển thread sang core khác | `isolcpus=`, `nohz_full=`, `rcu_nocbs=`, pin thread bằng `taskset`/`pthread_setaffinity_np` |
| Interrupt (IRQ) rơi vào core hot path | Chỉnh `/proc/irq/*/smp_affinity`, tắt `irqbalance` |
| CPU giảm xung / ngủ sâu (C-states) | BIOS: tắt C-states, P-states cố định; `cpupower frequency-set -g performance`; `intel_idle.max_cstate=0` |
| Hyper-threading chia sẻ core | Tắt SMT hoặc để sibling của core hot path trống |
| Page fault | `mlockall(MCL_CURRENT|MCL_FUTURE)`, pre-fault, huge pages |
| Transparent Huge Pages compaction | `echo never > /sys/kernel/mm/transparent_hugepage/defrag` hoặc dùng hugetlbfs tường minh |
| Timer tick | `nohz_full` |
| Swap | Tắt swap |
| Syscall trên hot path | Loại bỏ: không `printf`, không `clock_gettime` thừa (vDSO thì ổn) |

Tài liệu tốt nhất để bắt đầu: **Erik Rigtorp — Low latency tuning guide** (rigtorp.se).
Công cụ đo jitter hệ thống: `cyclictest` (rt-tests), `hwlatdetect`, `perf sched`.

## 2. Network stack — đường đi của gói tin

```
NIC → DMA vào ring buffer → IRQ/NAPI poll → driver → sk_buff → IP → UDP/TCP
    → socket buffer → (wake up) → recv() syscall → copy vào user space → app
```
Mỗi mũi tên là latency + jitter. Các cấp độ tối ưu:

1. **Kernel socket được tune**: `SO_BUSY_POLL`, `net.core.busy_poll`, busy-loop `recv` với
   `MSG_DONTWAIT`, tắt interrupt coalescing (`ethtool -C`), `TCP_NODELAY` (tắt Nagle),
   `TCP_QUICKACK`.
2. **epoll / io_uring**: giảm syscall, batch.
3. **AF_XDP**: kernel bypass "nửa vời" có sẵn trong Linux.
4. **Kernel bypass hoàn toàn**: Solarflare/AMD **OpenOnload** (thay socket API, không
   sửa code) và **ef_vi** (API mức thấp); **DPDK**; Mellanox/NVIDIA **VMA**.
5. **FPGA / SmartNIC**: logic chạy ngay trên card mạng.

## 3. Kiến thức giao thức cần biết
- **UDP multicast**: hầu hết sàn phát market data qua multicast; IGMP join; A/B feed
  (2 đường dư thừa) → arbitration; phát hiện gap qua sequence number; recovery qua TCP
  (snapshot/retransmission).
- **TCP** cho order entry: Nagle, delayed ACK, slow start, `SO_SNDBUF`.
- **Timestamping**: hardware timestamp của NIC (`SO_TIMESTAMPING`), PTP (`ptp4l`, `phc2sys`)
  để đồng bộ đồng hồ giữa các máy.

## 4. Công cụ
- `perf stat`, `perf record`, `perf top`, `perf c2c` (phát hiện false sharing!)
- `bpftrace` / BCC — tracing kernel
- `ethtool -S`, `ss -tinm`, `netstat -su` (đếm drop)
- `tcpdump` / Wireshark; `sockperf`, `netperf`, `qperf` để đo latency mạng

## 5. Bài tập
1. Chạy `cyclictest` trước và sau khi tune (isolcpus + governor performance). Ghi max latency.
2. Project 07: UDP ping-pong qua loopback rồi qua 2 máy, đo RTT histogram với: blocking
   `recv`, non-blocking busy-poll, `SO_BUSY_POLL`, (tùy chọn) io_uring, Onload nếu có card.
3. Dùng `perf c2c` để tìm false sharing trong thí nghiệm của Project 01.

## Đọc / xem
- Erik Rigtorp — *Low latency tuning guide* ⭐
- Brendan Gregg — *Systems Performance* (2nd ed.) ⭐ và brendangregg.com
- Red Hat — *Low Latency Performance Tuning for RHEL* (whitepaper)
- Packagecloud blog — *Monitoring and Tuning the Linux Networking Stack: Receiving Data* (và *Sending Data*) ⭐
- DPDK Programmer's Guide; tài liệu OpenOnload / ef_vi
- Cloudflare blog — các bài về kernel bypass, *How to receive a million packets per second*
