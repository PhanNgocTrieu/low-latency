# Module 00 — Bản đồ khái niệm low latency

**Mục tiêu:** có một "bản đồ" để biết mình đang thiếu khái niệm nào, khái niệm đó nằm ở
tầng nào của hệ thống, và nên học ở đâu trong repo này. Đây là trang tra cứu: đọc lướt một
lần, sau đó quay lại mỗi khi gặp thuật ngữ lạ.

Ký hiệu: ⭐ = gần như chắc chắn bị hỏi khi phỏng vấn / bắt buộc phải giải thích được.
Cột "Học ở đâu" trỏ tới module (`docs/0x`) hoặc project (`P0x`) liên quan.

---

## 0. Bức tranh tổng thể: một gói tin đi qua những đâu?

Mọi khái niệm bên dưới đều là một mảnh trên con đường **tick-to-trade**: từ lúc tín hiệu
thị trường chạm vào dây mạng đến lúc lệnh rời khỏi máy.

```
 dây mạng ──► NIC ──► (kernel hoặc kernel bypass) ──► thread market data
   [§7 Mạng]          [§6 OS, §7 Mạng]                 [§2 CPU, §3 Bộ nhớ, §4 C++]
                                                           │  SPSC queue  [§5 Concurrency]
                                                           ▼
                                                     thread strategy ──► risk check
                                                           │
                                                           ▼
                                                     thread gateway ──► NIC ──► sàn
 Đo toàn bộ quãng đường trên bằng: [§8 Đo lường]      Ghép lại thành hệ thống: [§9 Thiết kế]
```

Nguyên tắc chung: latency của cả đường đi = **tổng** latency từng chặng, còn jitter thì
cộng dồn theo kiểu "chặng tệ nhất thắng". Một chặng có p99 xấu là đủ làm hỏng cả hệ thống.

---

## 1. Các khái niệm nền (ngôn ngữ chung)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Latency vs throughput | Latency = một việc mất bao lâu; throughput = bao nhiêu việc mỗi giây | Tối ưu cho cái này thường làm hại cái kia (vd: batching tăng throughput nhưng tăng latency) |
| ⭐ Tail latency | Các percentile cao: p99, p99.9, max | Trong trading, lần chậm nhất mới quyết định mất cơ hội; trung bình che giấu nó |
| ⭐ Jitter | Độ dao động của latency giữa các lần | Hệ thống "trung bình 2 µs, thỉnh thoảng 200 µs" là hệ thống tệ |
| Determinism | Cùng input → cùng thời gian xử lý | Mục tiêu thật sự của low latency: nhanh *và ổn định* |
| ⭐ Hot path / cold path | Đường code chạy khi xử lý tick/lệnh vs mọi thứ còn lại | Chỉ hot path cần tối ưu cực đoan (xem docs/01) |
| ⭐ Tick-to-trade | Thời gian từ nhận dữ liệu thị trường đến gửi lệnh | Chỉ số chính của một hệ thống HFT (P08) |
| Critical path | Chuỗi bước phụ thuộc nhau dài nhất | Tối ưu chỗ khác không làm hệ thống nhanh hơn |
| Mechanical sympathy | Viết code "thuận" với cách phần cứng hoạt động | Triết lý xuyên suốt (Martin Thompson) |

---

## 2. CPU — bên trong một core (docs/02, P01)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Pipeline | Lệnh được chia thành nhiều giai đoạn chạy gối nhau như dây chuyền | Bất cứ thứ gì làm "xả" dây chuyền (đoán sai nhánh) đều tốn 15–20 cycle |
| Superscalar | Mỗi cycle core thực thi được nhiều lệnh (nhiều execution port) | Code có nhiều phép tính độc lập chạy nhanh hơn nhiều |
| ⭐ Out-of-order execution (OoO) | Core tự sắp xếp lại lệnh, chạy lệnh nào có đủ dữ liệu trước | Che được một phần cache miss, nhưng chỉ khi còn việc độc lập để làm |
| Dependency chain | Lệnh sau cần kết quả của lệnh trước | Chuỗi phụ thuộc dài = OoO không giúp được; latency cộng dồn |
| IPC | Instructions per cycle — số lệnh trung bình mỗi cycle | Thước đo nhanh: IPC thấp (<1) thường là đang chờ bộ nhớ |
| ⭐ Branch prediction | Core đoán trước nhánh `if` sẽ đi đường nào để chạy trước | Đoán sai ≈ 5 ns. P01 đo được chênh ~9–14 lần |
| Speculative execution | Chạy trước theo dự đoán, sai thì bỏ kết quả | Lý do branch prediction có lợi (và nguồn gốc lỗ hổng Spectre) |
| Branchless code | Thay `if` bằng phép tính (`cmov`, mask) | Thời gian không phụ thuộc dữ liệu → ổn định hơn |
| µop / µop cache (DSB) | Lệnh x86 được dịch ra micro-op; có cache riêng cho µop | Hot loop vừa µop cache thì front-end không thành nút cổ chai |
| Instruction cache (L1i) | Cache cho code, không phải dữ liệu | Code hot path bị phình (inline quá đà, template bùng nổ) → icache miss |
| ⭐ SIMD | Một lệnh xử lý nhiều phần tử (SSE/AVX/NEON) | Compiler tự vector hóa nếu code "sạch"; tăng tốc 4–16 lần |
| SMT / Hyper-threading | 2 thread logic dùng chung 1 core vật lý | Thread "hàng xóm" tranh cache và execution port → jitter |
| Frequency scaling (Turbo, P-states) | CPU tự đổi xung nhịp theo tải và nhiệt | Latency thay đổi theo xung → cố định tần số khi đo/chạy thật |
| C-states | Trạng thái ngủ của core khi rảnh | Đánh thức từ ngủ sâu mất hàng chục µs → lý do dùng busy-spin |
| Denormal (subnormal) float | Số thực cực nhỏ, xử lý chậm gấp ~100 lần | Bật FTZ/DAZ nếu code tính toán số thực trên hot path |

---

## 3. Bộ nhớ — cache, TLB, NUMA (docs/02, P01)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Memory hierarchy | Registers → L1 → L2 → L3 → DRAM; càng xa càng chậm (~1 → ~90 ns) | DRAM chậm hơn L1 khoảng 100 lần — kẻ thù số 1 |
| ⭐ Cache line | Đơn vị trao đổi của cache: 64 byte (Apple M: 128 byte) | Đọc 1 byte = tải cả line; layout dữ liệu quyết định số line phải tải |
| ⭐ Locality (spatial / temporal) | Dùng dữ liệu nằm gần nhau / vừa dùng xong | Mảng liên tục thắng linked list, `std::map` |
| ⭐ Hardware prefetcher | Phần cứng tự tải trước line khi thấy pattern đều | Duyệt tuần tự gần như "miễn phí"; truy cập ngẫu nhiên thì không |
| Set-associative, conflict miss | Mỗi địa chỉ chỉ được vào một "ngăn" (set) cố định | Stride lũy thừa 2 (4096 B) dồn mọi thứ vào cùng set → miss dù cache còn trống |
| Memory-level parallelism (MLP) | Core xử lý được nhiều cache miss song song (~10–16) | Pointer chasing tuần tự giết MLP: phải chờ miss trước xong mới biết địa chỉ sau |
| Write-back / store buffer | Lệnh ghi vào store buffer trước, rồi mới vào cache | Lý do x86 vẫn có thể đảo thứ tự Store→Load (docs/03) |
| ⭐ Cache coherence (MESI) | Giao thức giữ cho các core thấy cùng một giá trị | Ghi vào line mà core khác đang giữ = phải "giật" line về (~50–100 ns) |
| ⭐ False sharing | 2 biến độc lập chung 1 cache line, 2 core cùng ghi | Chậm gấp nhiều lần dù không chia sẻ logic; sửa bằng `alignas(64)` (P01) |
| ⭐ Virtual memory, TLB | Địa chỉ ảo → vật lý; TLB cache kết quả dịch | TLB miss = page walk thêm hàng chục ns (trong VM như WSL2 còn đắt hơn) |
| ⭐ Huge pages (2 MB / 1 GB) | Page lớn hơn → mỗi TLB entry phủ nhiều bộ nhớ hơn | Giảm TLB miss mạnh cho working set lớn |
| ⭐ Page fault | Truy cập trang chưa được map → kernel can thiệp (µs) | Pre-fault + `mlockall` lúc khởi động, không bao giờ trên hot path |
| ⭐ NUMA | Máy nhiều socket: RAM gắn với từng CPU, truy cập RAM "bên kia" chậm hơn | Pin thread và cấp phát bộ nhớ cùng một node (`numactl`) |
| Bandwidth vs latency | GB/s đọc được vs thời gian cho 1 lần đọc | Low latency hầu như luôn bị giới hạn bởi latency, không phải bandwidth |

---

## 4. C++ và compiler (docs/01, P03, P04)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Zero-cost abstraction | Trừu tượng hóa không tốn gì lúc chạy (template, inline) | Viết code đẹp mà vẫn nhanh — nếu biết compiler sinh ra gì |
| ⭐ Inlining | Compiler chèn thân hàm vào chỗ gọi | Mở đường cho mọi tối ưu khác; hàm không inline được là "bức tường" |
| ⭐ Virtual call vs static dispatch | Gọi qua vtable vs biết trước lúc compile (CRTP, template) | Virtual khó inline và có thể đoán sai nhánh gián tiếp |
| ⭐ Heap allocation trên hot path | `new`/`malloc`, `std::string`, `std::function` lớn... | Thời gian không xác định, có thể vào kernel; cấm trên hot path |
| ⭐ Memory pool / arena | Cấp phát trước một khối, tự quản lý bên trong | 0 malloc lúc chạy, dữ liệu nằm gần nhau (P04) |
| ⭐ Data-oriented design, AoS vs SoA | Tổ chức dữ liệu theo cách nó được *truy cập* | Field "nóng" nằm chung cache line; SoA dễ vector hóa |
| Intrusive container | Con trỏ liên kết nằm ngay trong object | Không cần cấp phát node riêng (P04) |
| Aliasing, `__restrict` | Compiler phải giả định 2 con trỏ có thể trỏ cùng chỗ | Giả định đó chặn vector hóa và giữ giá trị trong register |
| `[[likely]]`, `[[unlikely]]`, cold function | Gợi ý nhánh nào hay chạy | Giữ hot path gọn trong icache |
| Exceptions | Chi phí gần 0 khi không ném, rất đắt khi ném | Chỉ dùng cho lỗi thật sự, không dùng làm control flow |
| `-O2/-O3`, `-march=native` | Mức tối ưu và tập lệnh đích | `-march=native` mở AVX2..., nhưng binary chỉ chạy trên CPU tương tự |
| LTO, PGO | Tối ưu toàn chương trình; tối ưu theo profile chạy thật | Thường được thêm 5–20% miễn phí |
| ⭐ Undefined behavior | Code vi phạm luật C++, compiler được làm bất cứ gì | Data race trên biến không atomic là UB — lỗi khó tìm nhất |
| Godbolt / đọc assembly | Xem compiler thật sự sinh ra gì | Cách duy nhất để *biết* thay vì *đoán* |

---

## 5. Concurrency và lock-free (docs/03, P02, P06)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Data race | 2 thread truy cập cùng biến, ít nhất 1 ghi, không đồng bộ | UB trong C++; phải dùng `std::atomic` hoặc lock |
| ⭐ Atomic operation | Thao tác không thể bị "cắt ngang" | Nền tảng của mọi cấu trúc lock-free |
| ⭐ Memory ordering | `relaxed`, `acquire`, `release`, `seq_cst` | Quy định thứ tự các thao tác được thread khác nhìn thấy |
| ⭐ Happens-before | Quan hệ "A chắc chắn xảy ra trước B" giữa các thread | Cách chứng minh code concurrency đúng |
| x86 TSO vs ARM weak ordering | x86 giữ thứ tự gần đúng; ARM đảo nhiều hơn | Code sai có thể "chạy đúng" trên x86 nhưng hỏng trên ARM |
| ⭐ CAS (compare-and-swap) | "Nếu giá trị vẫn là X thì đổi thành Y" | Viên gạch của lock-free; tốn kém khi nhiều core tranh nhau |
| ABA problem | Giá trị đổi A→B→A, CAS tưởng không có gì thay đổi | Bẫy kinh điển trong lock-free stack/list |
| Lock-free vs wait-free | Hệ thống luôn tiến vs *mỗi* thread luôn tiến | Lock-free không có nghĩa là nhanh hơn; nó tránh bị chặn |
| ⭐ SPSC / MPSC / MPMC queue | Hàng đợi 1/nhiều producer, 1/nhiều consumer | SPSC là đơn giản và nhanh nhất — xương sống của pipeline trading (P02) |
| ⭐ Ring buffer | Mảng vòng kích thước cố định | Không cấp phát, thân thiện cache |
| Seqlock | Writer tăng bộ đếm phiên bản, reader thử lại nếu bị ghi chen | Phổ biến để chia sẻ snapshot (vd: giá) cho nhiều reader |
| Disruptor | Kiến trúc ring buffer của LMAX | Nguồn cảm hứng cho nhiều hệ thống low latency |
| ⭐ Busy-spin vs blocking | Vòng lặp kiểm tra liên tục vs ngủ chờ (mutex, condvar) | Spin: latency thấp nhất, đổi bằng 100% CPU một core |
| `PAUSE` / backoff | Lệnh gợi ý "tôi đang spin" | Giảm tiêu thụ tài nguyên của SMT sibling; tốn hàng chục–hàng trăm cycle |
| Contention / cache line bouncing | Nhiều core cùng ghi một line | Atomic có tranh chấp chậm gấp 10–100 lần atomic không tranh chấp |
| Share-nothing | Mỗi thread sở hữu dữ liệu riêng, giao tiếp bằng message | Tránh tận gốc mọi chi phí đồng bộ |

---

## 6. Hệ điều hành (docs/04)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Syscall | Gọi vào kernel (`read`, `write`, `send`...) | Vài trăm ns đến vài µs; loại khỏi hot path |
| vDSO | Một số "syscall" chạy ngay trong user space | `clock_gettime` qua vDSO ~20 ns, không vào kernel |
| ⭐ Context switch | Kernel đổi thread đang chạy trên core | Vài µs + làm nguội cache; nguồn jitter lớn |
| ⭐ CPU pinning / affinity | Gắn thread vào một core cố định | Cache luôn "ấm", không bị di chuyển |
| ⭐ `isolcpus`, `nohz_full`, `rcu_nocbs` | Cách ly core khỏi scheduler, timer tick, RCU callback | Core gần như chỉ chạy đúng thread của bạn |
| IRQ affinity | Chọn core xử lý interrupt phần cứng | Không để interrupt rơi vào core hot path |
| `SCHED_FIFO` | Chính sách scheduler real-time | Thread không bị thread thường chen ngang |
| `mlockall` | Khóa bộ nhớ trong RAM, không bị swap | Tránh page fault bất ngờ |
| Transparent Huge Pages (THP) | Kernel tự gộp page thành huge page | Tiện nhưng có thể gây stall khi compaction |
| SMI | Interrupt firmware mà OS không thấy | Gây stall µs–ms "bí ẩn"; phát hiện bằng `hwlatdetect` |
| Ảo hóa (VM, WSL2) | Thêm một lớp dịch địa chỉ và scheduler | Số đo jitter trong VM không phản ánh máy thật |

---

## 7. Mạng (docs/04, P05, P07)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Kernel network stack | NIC → interrupt → driver → TCP/IP → socket → app | Mỗi tầng thêm latency và jitter (vài µs) |
| Interrupt vs polling (NAPI, busy poll) | Chờ NIC báo hay chủ động hỏi liên tục | Polling bỏ được chi phí interrupt và đánh thức thread |
| ⭐ Kernel bypass | App đọc/ghi thẳng NIC, bỏ qua kernel (DPDK, OpenOnload/ef_vi, AF_XDP) | Từ ~5–10 µs xuống ~1 µs hoặc thấp hơn |
| ⭐ UDP multicast | Một gói gửi tới nhiều người nhận | Cách sàn phát market data; mất gói thì phải tự xử lý |
| Sequence number, gap recovery | Đánh số gói để phát hiện mất gói | Feed handler phải phát hiện gap và lấy lại (P05) |
| A/B feed arbitrage | Sàn gửi cùng dữ liệu trên 2 đường; lấy gói nào đến trước | Vừa giảm latency vừa chống mất gói |
| ⭐ `TCP_NODELAY` (tắt Nagle) | Gửi gói nhỏ ngay, không gom lại | Mặc định TCP có thể giữ gói hàng chục ms |
| Delayed ACK, `TCP_QUICKACK` | Bên nhận trì hoãn gửi ACK | Kết hợp với Nagle gây độ trễ khó hiểu |
| Zero-copy | Không copy dữ liệu giữa kernel và user | Bớt memcpy, bớt cache pollution |
| Serialization delay | Thời gian "đẩy" bit lên dây | 64 B ở 10 Gbps ≈ 50 ns; 1500 B ≈ 1.2 µs |
| Cut-through switch | Switch chuyển gói khi mới đọc header | Vài trăm ns thay vì chờ nhận đủ gói |
| Hardware timestamping, PTP | NIC gắn timestamp lúc gói tới; đồng bộ đồng hồ giữa máy | Cách duy nhất đo chính xác latency qua mạng |
| Tốc độ lan truyền | Cáp quang ~5 µs/km, vi ba (microwave) ~3.3 µs/km | Vì sao colocation và tuyến microwave tồn tại |
| FPGA / SmartNIC | Xử lý logic ngay trên phần cứng mạng | Tick-to-trade cỡ vài chục–vài trăm ns |
| Binary protocol (ITCH, SBE) | Message có layout cố định, đọc thẳng bằng offset | Parse gần như miễn phí so với text (FIX, JSON) |

---

## 8. Đo lường (docs/05)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Percentile, histogram | Phân phối latency thay vì một con số | Cách duy nhất để thấy tail latency |
| HdrHistogram | Histogram độ chính xác cao, chi phí ghi thấp | Chuẩn de facto để ghi latency |
| ⭐ Coordinated omission | Bộ đo vô tình bỏ sót đúng những lúc hệ thống chậm | Lỗi đo phổ biến nhất (Gil Tene) |
| ⭐ `rdtsc`, invariant TSC | Đọc bộ đếm cycle của CPU (~7–10 ns) | Đồng hồ rẻ nhất trên x86 |
| Warm-up | Bỏ các lần chạy đầu | Cache, TLB, branch predictor còn "lạnh" |
| `DoNotOptimize` | Chặn compiler xóa code benchmark | Không có nó, benchmark có thể đo... không gì cả |
| ⭐ Hardware performance counters | Đếm cache miss, branch miss, cycle... (`perf stat`) | Biết *vì sao* chậm, không chỉ *chậm bao nhiêu* |
| Top-down analysis (TMA) | Phân loại cycle: front-end, back-end, bad speculation, retiring | Phương pháp có hệ thống để tìm nút cổ chai |
| Flame graph | Biểu đồ thời gian theo call stack | Thấy ngay hàm nào chiếm nhiều thời gian |
| Amdahl's law | Tăng tốc bị giới hạn bởi phần không tối ưu được | Biết khi nào nên dừng tối ưu một chỗ |
| ⭐ Little's law | Số việc trong hệ thống = tốc độ đến × thời gian ở lại | Liên hệ throughput, latency, độ dài hàng đợi |
| ⭐ Queueing: utilization → latency | Gần 100% tải, thời gian chờ tăng vọt (M/M/1: ~1/(1−ρ)) | Hệ thống low latency phải chạy dư công suất xa |

---

## 9. Thiết kế hệ thống (docs/06, P08)

| Khái niệm | Hiểu ngắn gọn | Vì sao quan trọng |
|---|---|---|
| ⭐ Pre-allocation | Cấp phát mọi thứ lúc khởi động | Hot path không bao giờ cấp phát |
| ⭐ Single-threaded hot path | Mỗi chặng chạy trên 1 thread, 1 core | Không cần lock, dễ suy luận, dễ đo |
| ⭐ Cache warming | Định kỳ chạy "giả" đường gửi lệnh để giữ cache/predictor ấm | Lệnh thật hiếm khi gửi → lúc cần thì code đã nguội (Carl Cook) |
| Batching | Gom nhiều việc xử lý một lần | Tăng throughput, nhưng việc đầu tiên phải chờ |
| Backpressure | Báo cho bên gửi chậm lại khi bên nhận quá tải | Queue đầy thì phải có chính sách rõ ràng: chặn, bỏ, hay báo lỗi |
| Precomputation | Tính trước mọi thứ có thể (bảng tra, message mẫu) | Hot path chỉ còn điền vài field rồi gửi |
| Async logging | Hot path chỉ ghi dữ liệu thô vào queue; thread khác format | `printf` trên hot path tốn µs (P06) |
| Pre-trade risk check | Kiểm tra giới hạn trước khi gửi lệnh | Bắt buộc về pháp lý, nhưng phải chỉ tốn vài chục ns |
| Order book data structure | Cấu trúc lưu lệnh theo giá | Bài toán trung tâm: chọn cấu trúc theo pattern truy cập (P03) |

---

## Thứ tự học gợi ý

1. **§1 + §3**: latency numbers, cache line, locality → bạn đã bắt đầu với P01.
2. **§2**: pipeline, branch prediction, OoO, SIMD → đọc assembly trên godbolt.
3. **§4**: inlining, allocation, data-oriented design → P03, P04.
4. **§5**: memory model, SPSC → P02, P06.
5. **§8**: percentile, coordinated omission, `perf` → áp dụng cho mọi project từ đây.
6. **§6 + §7**: tuning Linux, kernel bypass → P05, P07 (cần Linux trên máy thật).
7. **§9**: ghép lại thành hệ thống → P08.

## Tự kiểm tra

Lấy ngẫu nhiên 10 khái niệm có ⭐ và giải thích thành tiếng trong 1 phút mỗi khái niệm,
không nhìn tài liệu. Mỗi khái niệm cần trả lời được 3 câu:

1. Nó là gì?
2. Vì sao nó làm chậm (hoặc nhanh) hệ thống?
3. Đo hoặc chứng minh nó bằng cách nào?

Khái niệm nào chưa trả lời được câu 3 → viết một thí nghiệm nhỏ trong P01 để tự đo.
