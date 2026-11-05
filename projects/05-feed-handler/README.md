# Project 05 — Market data feed handler (NASDAQ ITCH 5.0)

**Đọc kèm:** docs/06 (ITCH, MoldUDP64), docs/04 (UDP multicast)

Làm việc với **dữ liệu thật** — điểm cộng lớn trong mắt nhà tuyển dụng.

## Chuẩn bị dữ liệu
- Tải 1 file ITCH 5.0 mẫu từ `emi.nasdaq.com/ITCH/` (file `.gz`, giải nén vài GB, ~vài trăm
  triệu message). Đặt vào thư mục `data/` (đừng commit lên git).
- Đọc đặc tả **NASDAQ TotalView-ITCH 5.0** (PDF): định dạng từng loại message.
- Trong file, mỗi message có tiền tố 2 byte độ dài (big-endian).

## Giai đoạn A — Parser (offline)
1. `mmap` toàn bộ file (không `read` từng đoạn) → duyệt tuần tự.
2. Decode big-endian bằng `__builtin_bswap16/32/64` trên `memcpy` (tránh UB do unaligned).
   Timestamp là 6 byte.
3. Dispatch theo type bằng `switch` (compiler sinh jump table) — **không** dùng virtual/map.
4. Đếm số message mỗi loại, đo **msgs/giây** và **ns/message**.
   Mục tiêu tham khảo: > 100M msgs/s chỉ parse; vài chục M msgs/s khi dựng book.

## Giai đoạn B — Book builder
1. Dựng L3 book cho toàn bộ symbol (mảng book theo `stock_locate`, ID 16-bit có sẵn trong ITCH).
2. Xử lý `A, F, E, C, X, D, U` đúng ngữ nghĩa (`U` = xóa lệnh cũ + thêm lệnh mới).
3. Xuất top-of-book của 1 symbol (vd AAPL) mỗi khi BBO đổi → CSV → vẽ biểu đồ giá trong ngày.
4. Kiểm chứng: cuối ngày số lệnh còn lại hợp lý; không có lệnh âm khối lượng.

## Giai đoạn C — Network replay (Linux)
1. Tool `replayer`: đọc file, đóng gói theo **MoldUDP64** (session, sequence number,
   message count), gửi UDP multicast theo đúng nhịp timestamp (hoặc nhân tốc độ).
2. `feed_handler`: join multicast, busy-poll nhận, phát hiện **gap** qua sequence number,
   đẩy message qua SPSC queue (Project 02) sang thread book builder.
3. Bonus: 2 feed A/B, arbitration (lấy gói đến trước, bỏ trùng).
4. Đo latency nhận-gói → BBO cập nhật (histogram).

## Câu hỏi để viết blog
- Phần nào tốn thời gian nhất: decode, lookup order, cập nhật level?
- `mmap` vs `read` buffered khác nhau thế nào? Ảnh hưởng của page cache?
