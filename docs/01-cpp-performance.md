# Module 01 — C++ hiện đại cho hiệu năng

**Mục tiêu:** viết C++ mà bạn *biết* compiler sẽ sinh ra gì; tránh mọi chi phí ẩn trên
hot path (heap allocation, virtual call khó đoán, exception, lock, syscall).

## 1. Tư duy "hot path vs cold path"

- **Hot path**: từ lúc nhận gói tin thị trường → ra quyết định → gửi lệnh (tick-to-trade).
  Quy tắc: không malloc, không lock, không syscall, không exception, không I/O đồng bộ,
  không format chuỗi.
- **Cold path**: khởi tạo, cấu hình, xử lý lỗi, báo cáo. Viết rõ ràng là đủ.

Kỹ thuật tách: `[[likely]]/[[unlikely]]`, `__attribute__((cold, noinline))` cho hàm xử lý
lỗi, để hot path nhỏ gọn trong instruction cache.

## 2. Các chủ đề phải nắm

| Chủ đề | Cần hiểu được | Ghi chú low-latency |
|---|---|---|
| Value semantics, move | Khi nào copy/move, RVO/NRVO | Tránh copy struct lớn trên hot path |
| Templates & CRTP | Static polymorphism thay virtual | Inline được, không vtable lookup |
| `constexpr` / `consteval` | Tính toán lúc compile | Bảng tra, cấu hình cố định |
| `std::variant` vs virtual | Chi phí dispatch | `std::visit` thường sinh jump table |
| Memory layout | `sizeof`, alignment, padding, `alignas` | Nhóm field "nóng" vào cùng cache line |
| Containers | `vector`, `array`, `map`, `unordered_map` | `map`/`list` = pointer chasing → cache miss |
| Allocators | `std::pmr`, pool, arena | Pre-allocate lúc khởi động |
| Exceptions | Zero-cost khi không ném, rất đắt khi ném | Không ném trên hot path |
| Inlining | `inline`, `always_inline`, LTO | Kiểm chứng bằng godbolt/perf |
| Undefined behavior | Strict aliasing, signed overflow | Dùng `std::bit_cast`, `memcpy` |

## 3. Data-Oriented Design (DOD)

Khi chỉ duyệt vài field, cân nhắc SoA thay vì AoS:

```cpp
// AoS: duyệt price kéo theo cả 64 byte của mỗi Order vào cache
struct Order { uint64_t id; int64_t price; uint32_t qty; char pad[44]; };
std::vector<Order> orders;

// SoA: duyệt price chỉ tải đúng dữ liệu price
struct Orders { std::vector<uint64_t> id; std::vector<int64_t> price; std::vector<uint32_t> qty; };
```

## 4. Pattern hay gặp trong code HFT

- **Pre-allocation & object pool**: mọi bộ nhớ cấp phát lúc khởi động.
- **Fixed-point price**: giá là `int64_t` số tick, không dùng `double`.
- **Bảng tra theo index** thay vì hash map: symbol → `uint16_t` id lúc khởi động.
- **Branchless code** khi dữ liệu khó đoán.
- **Cache warming**: chạy "giả" hot path liên tục để code/dữ liệu luôn nóng (talk Carl Cook).
- **Compile-time configuration**: template theo sàn/chiến lược thay vì `if` runtime.

## 5. Bài tập

1. Trên godbolt.org so sánh assembly: virtual call, CRTP, `std::function`,
   `std::variant` + `visit`. Có indirect call không? Bao nhiêu lệnh?
2. Struct 7 field kích thước khác nhau: sắp xếp lại để `sizeof` nhỏ nhất, kiểm chứng
   bằng `static_assert`.
3. Benchmark duyệt 10 triệu phần tử: `vector` vs `list` vs `map`. Giải thích bằng cache.
4. Viết `parse_price("123.45") → int64_t 12345` không dùng `strtod`, không cấp phát.

## 6. Tự kiểm tra
- Vì sao `std::unordered_map` chậm hơn mong đợi? (node-based → cache miss; rehash cấp
  phát). Thay thế: open addressing (`absl::flat_hash_map`, `ankerl::unordered_dense`).
- Virtual call có thực sự chậm? (Predict đúng thì rẻ; vấn đề lớn là không inline được.)
- `-O2` vs `-O3` vs `-march=native`? PGO và LTO là gì?

## Đọc / xem
- Carl Cook — *When a Microsecond Is an Eternity* (CppCon 2017) ⭐
- David Gross — *When Nanoseconds Matter: Ultrafast Trading Systems in C++* (CppCon 2024) ⭐
- Matt Godbolt — *What Has My Compiler Done for Me Lately?* (CppCon 2017)
- Chandler Carruth — *Tuning C++: Benchmarks, and CPUs, and Compilers! Oh My!* (CppCon 2015)
- Mike Acton — *Data-Oriented Design and C++* (CppCon 2014)
- Agner Fog — *Optimizing software in C++* (PDF miễn phí)
- Sách: *C++ High Performance* (Andrist & Sehr)
