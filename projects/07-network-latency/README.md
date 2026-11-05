# Project 07 — Network latency lab (Linux)

**Đọc kèm:** docs/04, docs/05. **Cần Linux.** Tốt nhất là 2 máy nối trực tiếp.

## Phần 1 — UDP ping-pong
- `server`: nhận gói UDP, gửi lại ngay. `client`: gửi gói 64 byte có sequence + timestamp,
  đo RTT từng gói, ghi histogram.
- Tải gửi theo lịch cố định (vd 10k pps) để tránh coordinated omission (docs/05).
- So sánh các chế độ (giữ mọi thứ khác cố định):

| Chế độ | p50 | p99 | p99.9 | max |
|---|---|---|---|---|
| Blocking `recvfrom` | | | | |
| Non-blocking busy-poll (`MSG_DONTWAIT`) | | | | |
| + `SO_BUSY_POLL` / `net.core.busy_poll` | | | | |
| + thread pin + `isolcpus` + governor performance | | | | |
| + IRQ affinity, tắt interrupt coalescing | | | | |
| io_uring (tùy chọn) | | | | |
| OpenOnload / AF_XDP (nếu có phần cứng) | | | | |

## Phần 2 — TCP
- Lặp lại với TCP; quan sát ảnh hưởng của Nagle và delayed ACK; bật `TCP_NODELAY`, `TCP_QUICKACK`.

## Phần 3 — Multicast
- Gửi/nhận multicast trên LAN (hoặc loopback với `IP_MULTICAST_LOOP`), dùng làm nền cho
  Project 05 giai đoạn C.

## Phần 4 — Timestamp phần cứng (nếu NIC hỗ trợ)
- `SO_TIMESTAMPING` lấy thời điểm gói tới NIC → tách latency mạng khỏi latency ứng dụng.

So sánh kết quả của bạn với `sockperf ping-pong` để kiểm chứng công cụ tự viết.
