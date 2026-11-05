# Project 08 — Capstone: hệ thống tick-to-trade end-to-end

Ghép tất cả thành một hệ thống hoàn chỉnh. Đây là repo bạn ghim đầu tiên trên GitHub và
nói đến trong mọi buổi phỏng vấn.

## Kiến trúc
```
┌──────────────┐ multicast  ┌──────────────┐ SPSC ┌───────────┐ SPSC ┌───────────┐ TCP  ┌──────────────┐
│ Exchange sim │──────────► │ Feed handler │────► │ Strategy  │────► │ Order GW  │────► │ Exchange sim │
│ (replay ITCH │  (P05-C)   │ + book (P03) │      │ + risk    │      │ (OUCH-like│      │ matching     │
│  + matching) │ ◄──────────┼──────────────┼──────┼───────────┼──────│  binary)  │ ◄────│ engine (P03) │
└──────────────┘  exec rpt  └──────────────┘      └───────────┘      └───────────┘      └──────────────┘
              Logger bất đồng bộ (P06) ・ Pool allocator (P04) ・ Thread pinning (P07)
```

## Yêu cầu
1. **Exchange simulator**: phát market data (replay ITCH hoặc tự sinh) + nhận lệnh qua TCP
   bằng giao thức nhị phân tự định nghĩa (lấy cảm hứng từ OUCH), khớp bằng matching engine
   của Project 03, trả execution report.
2. **Trading system** (process riêng, mỗi thành phần hot path 1 thread pinned):
   - Feed handler + book builder.
   - Strategy đơn giản (vd: khi spread > X tick thì đặt lệnh giữa spread; hoặc bám theo
     imbalance top-of-book). Mục đích là *đường đi dữ liệu*, không phải lợi nhuận.
   - Pre-trade risk: max order size, max position, price band, rate limit — tất cả O(1).
   - Order gateway: encode lệnh, theo dõi trạng thái lệnh (new → ack → fill/cancel).
3. **Đo tick-to-trade**: timestamp khi gói market data tới (lý tưởng: HW timestamp) và khi
   lệnh được `send()` → histogram. Bóc tách latency từng chặng (decode, book, strategy,
   risk, encode, send).
4. **Không malloc, không syscall (trừ send/recv), không lock** trên hot path — chứng minh bằng
   `strace -c`, counter `operator new`, và `perf`.
5. Cấu hình bằng file (cold path), log qua async logger.
6. CI (GitHub Actions): build + test + sanitizer.

## Sản phẩm cuối
- README có sơ đồ, hướng dẫn chạy 1 lệnh, bảng latency từng chặng, flame graph.
- Bài blog: "Xây dựng hệ thống tick-to-trade dưới X µs bằng C++ — những gì tôi học được".
- Danh sách "nếu có thêm thời gian": kernel bypass, FPGA, SBE, gap recovery đầy đủ…

## Tham khảo
- Sourav Ghosh — *Building Low Latency Applications with C++* (cấu trúc tương tự, dùng để
  đối chiếu sau khi tự thiết kế — đừng chép).
