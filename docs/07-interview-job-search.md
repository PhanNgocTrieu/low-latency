# Module 07 — Phỏng vấn & tìm việc remote

## 1. Thực tế thị trường (đọc kỹ)
- Các quỹ HFT/prop lớn (Jane Street, Citadel Securities, Hudson River Trading, Jump,
  Optiver, IMC, Tower, Virtu…) **phần lớn làm việc tại văn phòng** (London, NYC, Chicago,
  Amsterdam, Singapore, Hong Kong, Sydney). Remote 100% ở các công ty này hiếm.
- Nơi **remote dễ hơn**:
  - Crypto market makers, sàn crypto, công ty hạ tầng blockchain hiệu năng cao.
  - Prop firm / quỹ nhỏ, công ty trading boutique.
  - Vendor cho ngành tài chính: market data, OMS/EMS, phần mềm giao dịch, trading platform.
  - Ngành "lân cận" cần cùng kỹ năng: database hiệu năng cao, message broker/streaming,
    ad-tech (real-time bidding), game server, networking/telecom, HPC.
  → Luôn kiểm tra lại chính sách remote trong từng tin tuyển dụng.
- Từ Việt Nam: múi giờ hợp với các công ty APAC (Singapore, HK, Sydney, Tokyo) và châu Âu
  (lệch 5–6 giờ). Nhiều công ty thuê dạng contractor hoặc qua EOR (Deel, Remote.com…).
- Lộ trình thực tế: remote ở công ty lân cận/crypto → tích lũy kinh nghiệm → cân nhắc
  relocate (Singapore là điểm đến phổ biến) nếu muốn vào HFT top-tier.

## 2. Các vị trí và từ khóa tìm kiếm
`low latency C++`, `HFT developer`, `trading systems engineer`, `market data engineer`,
`execution engineer`, `C++ performance engineer`, `core infrastructure (trading)`,
`exchange connectivity`, `FPGA engineer` (nhánh khác, cần Verilog/VHDL).

Nơi tìm: LinkedIn (lọc Remote), eFinancialCareers, Hacker News "Who is hiring" hàng tháng,
các job board crypto (cryptojobslist, web3.career), We Work Remotely, RemoteOK, và
headhunter chuyên ngành tài chính/quant trên LinkedIn.

## 3. Hồ sơ & portfolio
- **CV**: dẫn số liệu. Không viết "tối ưu hiệu năng" — viết "giảm p99 order book update từ
  850 ns xuống 120 ns bằng cấu trúc mảng phẳng và pool allocator".
- **GitHub** (từ các project trong repo này), mỗi repo:
  README → mục tiêu, kiến trúc, cách build, **bảng/biểu đồ latency**, môi trường đo
  (CPU, OS, flag compile), nhật ký tối ưu, hạn chế.
- **Blog** 2–3 bài sâu > 20 bài nông. Gợi ý: "Tôi đã tối ưu SPSC queue từ 20M lên 200M ops/s
  như thế nào", "Dựng order book từ dữ liệu ITCH thật".
- Đóng góp open-source (nhỏ cũng được) vào dự án liên quan: logger, queue, thư viện FIX…

## 4. Quy trình phỏng vấn thường gặp
1. Online test (HackerRank/Codility): thuật toán + C++.
2. Phone screen kỹ thuật: C++ sâu + kiến thức hệ thống.
3. Onsite/virtual 3–5 vòng: coding (thường C++ thuần, không STL lạ), system design
   (thiết kế matching engine/feed handler), low-level (cache, OS, network), behavioral.
4. Một số nơi: take-home (vd viết order book và tối ưu).

## 5. Ngân hàng câu hỏi (tự luyện, trả lời thành tiếng)

### C++
1. Chi phí của virtual function? Khi nào compiler devirtualize được?
2. `std::move` thực sự làm gì? Khi nào move chậm hơn copy?
3. RVO/NRVO, copy elision bắt buộc từ C++17.
4. CRTP là gì, dùng để làm gì trong low latency?
5. `inline` nghĩa là gì (cả về ODR lẫn tối ưu)?
6. Vì sao `std::shared_ptr` có chi phí? (control block, atomic refcount)
7. Strict aliasing là gì? Type punning an toàn thế nào?
8. `std::vector::push_back` amortized O(1) nhưng tại sao có thể gây spike latency?
9. Exception zero-cost là gì? Vì sao vẫn tránh trên hot path?
10. `constexpr` vs `consteval` vs `constinit`.
11. Placement new, alignment, `std::launder`.
12. Thiết kế `small_vector` / `inplace_function`.

### Phần cứng & OS
13. Giải thích cache hierarchy và con số latency mỗi tầng.
14. False sharing là gì? Phát hiện và sửa thế nào?
15. Vì sao duyệt `std::list` chậm hơn `std::vector` dù cùng O(n)?
16. Branch misprediction; viết lại một đoạn code thành branchless.
17. TLB, huge pages, khi nào có lợi?
18. Context switch tốn gì? Syscall tốn gì? vDSO là gì?
19. NUMA ảnh hưởng thế nào? Đặt thread và bộ nhớ ra sao?
20. Tại sao busy-spin thay vì condition variable?
21. `isolcpus`, `nohz_full`, IRQ affinity dùng để làm gì?
22. Page fault minor vs major. Làm sao để hot path không bao giờ page fault?

### Concurrency
23. Giải thích acquire/release. Vẽ ví dụ hỏng nếu dùng relaxed.
24. Viết SPSC ring buffer trên bảng trắng. Tối ưu thế nào?
25. ABA problem và cách giải.
26. Seqlock hoạt động thế nào? Hạn chế?
27. Lock-free có luôn nhanh hơn mutex không?
28. x86 TSO khác ARM thế nào về memory ordering?

### Mạng
29. Đường đi của gói tin UDP từ NIC đến ứng dụng.
30. Kernel bypass là gì? So sánh Onload, ef_vi, DPDK, AF_XDP.
31. Nagle, delayed ACK, `TCP_NODELAY`.
32. Multicast hoạt động thế nào? Xử lý mất gói (gap) trong market data ra sao?
33. Hardware timestamping và PTP để làm gì?

### Đo lường
34. Đo latency 1 hàm 50 ns thế nào cho chính xác?
35. Coordinated omission là gì?
36. Vì sao báo cáo p99.9 và max chứ không chỉ mean?
37. Dùng `perf` tìm bottleneck ra sao?

### System design
38. Thiết kế matching engine: cấu trúc dữ liệu cho price level và order lookup, độ phức tạp từng thao tác.
39. Thiết kế feed handler với A/B arbitration và gap recovery.
40. Thiết kế logger không làm chậm hot path.
41. Thiết kế pre-trade risk check < 100 ns.
42. Hệ thống chạy cả ngày nhưng thỉnh thoảng có spike 50 µs — bạn điều tra thế nào?
43. Thiết kế hệ thống replay market data để backtest giống hệt production.

### Coding (luyện C++ thuần)
44. LRU cache O(1) không cấp phát động.
45. Ring buffer cố định dung lượng (template).
46. Parse số nguyên/giá từ chuỗi nhanh nhất có thể.
47. Order book: add/cancel/modify/execute + top of book.
48. Tìm median trong stream (2 heap).
49. Bitset / popcount tricks, lũy thừa 2, căn lề.
50. Hash map open addressing tự viết.

Luyện thêm LeetCode mức Medium (~100 bài) bằng C++, tập trung array, hash, heap, linked list, design.
