# Tài nguyên học tập (đã chọn lọc)

⭐ = ưu tiên cao. Đọc/xem theo thứ tự lộ trình trong README.

## Sách
| Sách | Dùng cho | Ghi chú |
|---|---|---|
| ⭐ Bryant & O'Hallaron — *Computer Systems: A Programmer's Perspective* | Nền tảng | Ch. 5 (tối ưu), 6 (bộ nhớ), 9 (virtual memory), 12 (concurrency) |
| ⭐ Ulrich Drepper — *What Every Programmer Should Know About Memory* | Cache/RAM | PDF miễn phí |
| ⭐ Denis Bakhvalov — *Performance Analysis and Tuning on Modern CPUs* | Đo lường, perf | Miễn phí, rất thực tế |
| ⭐ Anthony Williams — *C++ Concurrency in Action* (2nd ed.) | Concurrency | Ch. 5 memory model, ch. 7 lock-free |
| ⭐ Sourav Ghosh — *Building Low Latency Applications with C++* | Trading system | Dựng hệ thống trading hoàn chỉnh |
| ⭐ Brendan Gregg — *Systems Performance* (2nd ed.) | Linux | Tra cứu khi làm module 04 |
| ⭐ Larry Harris — *Trading and Exchanges* | Nghiệp vụ | Phần I–II |
| Andrist & Sehr — *C++ High Performance* (2nd ed.) | C++ | |
| Donadio, Ghosh, Rossier — *Developing High-Frequency Trading Systems* | Trading | Tổng quan, có C++/Java/Python |
| Paul McKenney — *Is Parallel Programming Hard…?* | Concurrency nâng cao | Miễn phí |
| Herlihy & Shavit — *The Art of Multiprocessor Programming* | Lý thuyết lock-free | Học thuật |
| Agner Fog — *Optimization manuals* (5 tập) | Tra cứu | Miễn phí, agner.org |
| Barry Johnson — *Algorithmic Trading & DMA* | Nghiệp vụ | Order types, DMA |

## Talk (YouTube)
- ⭐ Carl Cook — *When a Microsecond Is an Eternity: High Performance Trading Systems in C++* (CppCon 2017)
- ⭐ David Gross — *When Nanoseconds Matter: Ultrafast Trading Systems in C++* (CppCon 2024)
- ⭐ Gil Tene — *How NOT to Measure Latency*
- ⭐ Herb Sutter — *atomic<> Weapons* (2 phần)
- Timur Doumler — *Want fast C++? Know your hardware!* (CppCon 2016)
- Chandler Carruth — *Tuning C++: Benchmarks, and CPUs, and Compilers! Oh My!* (CppCon 2015)
- Chandler Carruth — *Efficiency with Algorithms, Performance with Data Structures* (CppCon 2014)
- Mike Acton — *Data-Oriented Design and C++* (CppCon 2014)
- Matt Godbolt — *What Has My Compiler Done for Me Lately?* (CppCon 2017)
- Fedor Pikus — *The speed of concurrency (is lock-free faster?)* (CppCon 2016)
- Martin Thompson — các talk về *Mechanical Sympathy* và *Aeron*
- Tìm thêm: kênh CppCon, Meeting C++, C++Now với từ khóa "low latency", "trading", "HFT".

## Blog / website
- ⭐ rigtorp.se — Erik Rigtorp (low latency tuning guide, SPSC/MPMC queue)
- ⭐ brendangregg.com — perf, flame graph, Linux performance
- mechanical-sympathy.blogspot.com — Martin Thompson
- 1024cores.net — Dmitry Vyukov (lock-free)
- lemire.me/blog — Daniel Lemire (SIMD, parse nhanh)
- easyperf.net — Denis Bakhvalov
- preshing.com — Jeff Preshing (memory ordering, giải thích rất dễ hiểu) ⭐
- blog.packagecloud.io — bài về Linux networking stack
- blog.cloudflare.com — networking hiệu năng cao
- Jane Street Tech Blog — văn hóa kỹ thuật của một công ty trading

## Mã nguồn nên đọc
| Repo | Học được gì |
|---|---|
| rigtorp/SPSCQueue, rigtorp/MPMCQueue | Queue lock-free gọn, chuẩn |
| facebook/folly (`ProducerConsumerQueue`, `MPMCQueue`) | Code production |
| LMAX-Exchange/disruptor | Ring buffer pattern (Java) |
| real-logic/aeron | Messaging low latency (Java/C++) |
| odygrd/quill | Logger low latency C++ |
| PlatformLab/NanoLog | Logger nanosecond, deferred formatting |
| cameron314/concurrentqueue | MPMC queue phổ biến |
| abseil/abseil-cpp (`flat_hash_map`) | Hash map Swiss table |
| martinus/unordered_dense | Hash map nhanh, header-only |
| fmtlib/fmt | Format nhanh |
| objectcomputing/liquibook | Order book & matching engine |
| google/benchmark, martinus/nanobench | Micro-benchmark |
| HdrHistogram/HdrHistogram_c | Histogram latency |
| scylladb/seastar | Thread-per-core, share-nothing |
| DPDK, Xilinx-CNS/onload (OpenOnload) | Kernel bypass |

## Công cụ
godbolt.org (Compiler Explorer) · quick-bench.com · perf · Intel VTune · pmu-tools (toplev) ·
valgrind/cachegrind · ThreadSanitizer/AddressSanitizer · bpftrace · Wireshark · rt-tests (cyclictest)

## Dữ liệu
- NASDAQ ITCH 5.0 sample files: `emi.nasdaq.com/ITCH/`
- Crypto: WebSocket API công khai của các sàn (depth/trade streams), Tardis.dev (dữ liệu lịch sử, có phí, có mẫu miễn phí)
