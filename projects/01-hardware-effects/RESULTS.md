# Kết quả tham khảo — Apple M1 Pro, macOS, Apple clang 21, `-O2 -mcpu=native`

(2026-11-05, lần chạy đầu. Hãy chạy lại và thêm kết quả của bạn, nhất là trên Linux x86.)

## 1. Memory hierarchy
| Working set | ns/hop | Diễn giải |
|---|---|---|
| 16–128 KB | ~1.2–1.6 | Vừa L1D (P-core M1 Pro: 128 KB) |
| 256 KB – 4 MB | ~5.6 | L2 (12 MB chia sẻ trong cụm P-core) |
| 8 MB | ~12 | Gần đầy L2 + bắt đầu có TLB miss |
| 16 MB | ~47 | Tràn L2 → System Level Cache (SLC) |
| 32 MB+ | ~110–145 | DRAM + TLB miss |

→ Từ L1 đến DRAM chênh **~100 lần**. Đây là lý do "cache miss" là kẻ thù số 1.

## 2. Branch prediction
| Biến thể | ns/phần tử |
|---|---|
| branchy, dữ liệu ngẫu nhiên | 3.29 |
| branchy, dữ liệu đã sort | 0.38 |
| branchless | 0.06 |

→ Đoán sai nhánh làm chậm **~9 lần**. Bản branchless còn nhanh hơn nữa vì compiler đã
**vector hóa** (NEON) vòng lặp — kiểm chứng bằng godbolt.

## 3. False sharing
| Bố trí | Thời gian |
|---|---|
| 2 counter chung cache line (16 byte) | 586 ms |
| 2 counter cách nhau 128 byte | 107 ms |

→ **~5.5 lần** chậm hơn chỉ vì 2 biến *độc lập* nằm cạnh nhau.
