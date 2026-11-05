# Module 06 — Kiến thức nghiệp vụ trading

**Mục tiêu:** hiểu đủ nghiệp vụ để nói chuyện với trader/quant và thiết kế đúng hệ thống.
Bạn không cần làm quant, nhưng phải hiểu order book và luồng dữ liệu.

## 1. Thuật ngữ cốt lõi
- **Order book (LOB)**: danh sách lệnh mua (bid) và bán (ask) chờ khớp, sắp theo giá.
- **Best bid / best ask (BBO, top of book)**, **spread** = ask − bid, **mid price**.
- **Tick size**: bước giá nhỏ nhất. **Lot size**: bước khối lượng nhỏ nhất.
- **Price-time priority (FIFO)**: cùng giá → lệnh đến trước khớp trước. Một số sàn dùng pro-rata.
- **Maker / taker**: lệnh nằm chờ trên sổ (thêm thanh khoản) vs lệnh khớp ngay (lấy thanh khoản).
- **Loại lệnh**: limit, market, IOC (immediate-or-cancel), FOK (fill-or-kill), post-only, iceberg.
- **Market making**: đặt cả bid và ask, kiếm spread, quản lý tồn kho (inventory risk).
- **Arbitrage**: cùng tài sản lệch giá giữa các sàn → tốc độ quyết định ai thắng.
- **Tick-to-trade**: thời gian từ khi nhận tick thị trường đến khi lệnh rời khỏi máy.
- **Colocation**: đặt máy chủ trong datacenter của sàn.
- **Pre-trade risk check**: giới hạn khối lượng, giá, vị thế — bắt buộc, và phải nhanh.

## 2. Market data: L1 / L2 / L3
| Mức | Nội dung | Ví dụ |
|---|---|---|
| L1 | BBO + giao dịch cuối | Quote feed |
| L2 | Tổng khối lượng theo từng mức giá (market-by-price) | CME MDP 3.0 (book depth) |
| L3 | Từng lệnh riêng lẻ (market-by-order) | **NASDAQ TotalView-ITCH 5.0** |

**NASDAQ ITCH 5.0** là giao thức nhị phân, big-endian, các message:
`A`/`F` Add Order, `E`/`C` Order Executed, `X` Order Cancel, `D` Order Delete,
`U` Order Replace, `P` Trade, `S` System Event, `R` Stock Directory…
Truyền qua **MoldUDP64** (UDP multicast + sequence number). NASDAQ công bố file ITCH mẫu
cả ngày giao dịch (vài GB) tại `emi.nasdaq.com/ITCH/` — dùng cho Project 05.

## 3. Order entry
- **FIX protocol**: dạng text `tag=value`, phổ biến nhưng chậm để parse.
  Học FIX 4.2/4.4: `NewOrderSingle (35=D)`, `ExecutionReport (35=8)`, `OrderCancelRequest (35=F)`.
- **Giao thức nhị phân của sàn**: NASDAQ **OUCH**, CME **iLink 3** (SBE encoding), …
- **SBE (Simple Binary Encoding)**: định dạng encode nhanh, dùng nhiều trong trading.

## 4. Kiến trúc một hệ thống trading điển hình
```
Exchange ──multicast──► Feed Handler ──► Book Builder ──► Strategy ──► Risk Check ──► Order Gateway ──TCP──► Exchange
                         (decode,          (L3 → L2/L1)    (signal,      (limits)       (encode OUCH/FIX,
                          gap detect,                       quote)                        track order state)
                          A/B arbitration)
                                           └──────── Logging / Monitoring / PnL (off hot path) ────────┘
```

## 5. Crypto — lối vào dễ cho remote
Sàn crypto (Binance, OKX, Coinbase, Bybit, Deribit…) cho phép bất kỳ ai kết nối qua
WebSocket/REST (một số có FIX). Latency cỡ ms nhưng là sân chơi tốt để làm project thật:
nhận order book stream, giữ book cục bộ, đo latency, viết bot paper-trading. Nhiều công ty
crypto market making tuyển remote.

## 6. Bài tập
1. Project 03: matching engine price-time priority.
2. Project 05: dựng L3 book từ file ITCH thật; in top-of-book của AAPL theo thời gian.
3. Viết parser FIX tối giản (không cấp phát) cho `35=D` và `35=8`.
4. (Crypto) Kết nối WebSocket depth stream của một sàn, duy trì book cục bộ, kiểm tra
   checksum/sequence theo tài liệu của sàn.

## Đọc / xem
- Larry Harris — *Trading and Exchanges: Market Microstructure for Practitioners* ⭐ (đọc phần I–II)
- Sourav Ghosh — *Building Low Latency Applications with C++* (Packt, 2023) ⭐ — dựng cả hệ thống trading C++ từ đầu
- Donadio, Ghosh, Rossier — *Developing High-Frequency Trading Systems* (Packt, 2022)
- Barry Johnson — *Algorithmic Trading & DMA*
- Đặc tả: *NASDAQ TotalView-ITCH 5.0*, *MoldUDP64*, *OUCH 4.2/5.0*, *FIX 4.4*
- Michael Lewis — *Flash Boys* (đọc giải trí, lấy bối cảnh)
