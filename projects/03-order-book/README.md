# Project 03 — Order book & matching engine

**Đọc kèm:** [docs/06-trading-domain.md](../../docs/06-trading-domain.md), [docs/01-cpp-performance.md](../../docs/01-cpp-performance.md)

Đây là project "trung tâm" của portfolio: gần như mọi công ty trading đều hỏi thiết kế
order book. Bạn sẽ có một bản baseline đúng và đo được, rồi tối ưu dần.

```bash
make test-03   # unit test + differential test
make run-03    # benchmark 5 triệu thao tác
```

## Có sẵn
- `order_book.hpp` — `MapOrderBook`: `std::map` cho mức giá, `std::list` cho FIFO,
  `std::unordered_map` cho tra cứu id. Đúng nhưng nhiều malloc và pointer chasing.
- `test.cpp` — unit test + **differential test** (so 2 cài đặt trên chuỗi lệnh ngẫu nhiên).
- `bench.cpp` — luồng lệnh tổng hợp quanh mid price, đo từng thao tác.

## Nhiệm vụ: viết `FastOrderBook`
Cùng interface (`add_limit`, `cancel`, `best_bid`, `best_ask`, `volume_at`, `order_count`).

Gợi ý theo thứ tự (đo sau mỗi bước, ghi vào bảng bên dưới):
1. **Mảng mức giá phẳng**: giá nằm trong dải hữu hạn quanh mid → `std::vector<Level>`
   đánh index theo `px - base_price`. Theo dõi `best_bid_idx`/`best_ask_idx`; khi mức tốt
   nhất rỗng, quét tới mức kế tiếp (thường rất gần).
2. **Lưu tổng khối lượng mỗi mức** → `volume_at` O(1).
3. **Order pool + intrusive doubly-linked list**: mỗi `Order` có `prev/next` (index 32-bit
   thay vì con trỏ), nằm trong mảng cấp phát sẵn; free-list để tái sử dụng (→ Project 04).
4. **Tra cứu id**: nếu id tăng dần và dày đặc → mảng trực tiếp `id → order index`;
   nếu không → hash map open addressing.
5. **Layout**: struct `Order` gọn (≤ 32 byte), field nóng đặt cạnh nhau.
6. Bỏ `std::function`/lambda nặng; callback là template.

Mục tiêu tham khảo: p50 giảm ≥ 5x, p99.9 giảm ≥ 10x so với baseline; **0 malloc** sau khởi
động (kiểm tra bằng cách đếm trong `operator new` override, hoặc Instruments/heaptrack).

## Mở rộng nghiệp vụ
- `modify` (giảm khối lượng giữ ưu tiên; đổi giá mất ưu tiên), lệnh `IOC`, `FOK`, market order.
- Snapshot L2 top-N mức giá.
- Nhiều symbol: mảng các book, symbol id `uint16_t`.

## Nhật ký kết quả

Máy: ______ · OS: ______ · Compiler/flags: ______

| Phiên bản | M ops/s | p50 | p99 | p99.9 | max | Thay đổi |
|---|---|---|---|---|---|---|
| MapOrderBook | | | | | | baseline |
| *tham khảo M1 Pro: Map* | 5.83 | 83 | 625 | 4167 | 5.5 ms | p99.9 cao do malloc + rehash của unordered_map |
| Fast v1 | | | | | | mảng mức giá |
| Fast v2 | | | | | | + pool + intrusive list |
