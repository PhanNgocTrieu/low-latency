# Project 01 — Hardware effects

**Đọc kèm:** [docs/02-computer-architecture.md](../../docs/02-computer-architecture.md), [docs/05-measurement.md](../../docs/05-measurement.md)

## Mục tiêu
Tự đo ba hiệu ứng phần cứng quyết định latency, thay vì tin vào con số trên mạng:
1. **Memory hierarchy**: latency L1 / L2 / L3 / DRAM bằng pointer chasing ngẫu nhiên.
2. **Branch prediction**: chi phí đoán sai nhánh; so với code branchless.
3. **False sharing**: 2 thread ghi biến khác nhau nhưng chung cache line.

```bash
make run-01                      # chạy cả 3
./build/01-hardware-effects cache
```

## Việc cần làm
- [ ] Chạy, chép kết quả vào `RESULTS.md` kèm thông tin máy (CPU, OS, compiler, flags).
- [ ] Vẽ biểu đồ ns/hop theo working set (Python matplotlib hoặc Google Sheets), đối chiếu với
      kích thước cache thật của CPU (`sysctl -a | grep cache` trên Mac, `lscpu` trên Linux).
- [ ] Giải thích bằng lời của bạn: vì sao có "bậc thang"? Vì sao bậc DRAM nhảy cao hơn
      trung bình ở working set rất lớn (gợi ý: TLB miss)?
- [ ] Bỏ dòng `asm volatile("")` trong `sum_branchy`, xem assembly trên godbolt (`-O2`),
      đo lại. Compiler đã làm gì?
- [ ] Thêm thí nghiệm: duyệt mảng 2D theo hàng vs cột; stride access.
- [ ] Thêm thí nghiệm: đọc tuần tự (sequential) vs ngẫu nhiên cùng working set → thấy tác
      dụng của prefetcher.
- [ ] (Linux) Chạy dưới `perf stat -e cache-misses,branch-misses,dTLB-load-misses` và
      `perf c2c record` cho phần false sharing.

## Kết quả tham khảo
Xem `RESULTS.md` sau khi chạy lần đầu (được tạo sẵn với kết quả trên máy M1 Pro).
