# Project 04 — Memory pool & intrusive containers

**Đọc kèm:** docs/01 (Allocators), docs/02 (Cache, TLB)

## Vì sao
`malloc`/`new` có latency không xác định (lock nội bộ, tìm free block, có thể gọi `mmap`
→ page fault). Hệ thống low latency cấp phát toàn bộ lúc khởi động và tái sử dụng.

## Yêu cầu
1. `ObjectPool<T>`: dung lượng cố định, `T* allocate(args...)` / `void deallocate(T*)`
   O(1), dùng **free-list nhúng** (con trỏ next nằm trong chính slot trống).
2. Bộ nhớ của pool: cấp phát 1 khối liên tục, căn lề cache line, **pre-fault** (ghi vào mọi
   trang), tùy chọn huge pages (Linux: `mmap` + `MAP_HUGETLB` hoặc `madvise(MADV_HUGEPAGE)`).
3. `IntrusiveList<T>`: node chứa `prev/next` (dùng index `uint32_t` thay vì con trỏ để
   struct nhỏ hơn).
4. Áp dụng vào `FastOrderBook` của Project 03.
5. Công cụ kiểm tra **0 allocation trên hot path**: override `operator new` toàn cục để
   đếm số lần gọi; assert = 0 trong vòng lặp benchmark.

## Đo
- Benchmark: `new/delete` vs `ObjectPool` cho 10 triệu cặp allocate/free với pattern ngẫu nhiên.
  So sánh p50/p99/max.
- (Linux) `perf stat -e page-faults,dTLB-load-misses` có/không pre-fault và huge pages.

## Mở rộng
- Arena/bump allocator cho dữ liệu sống theo "phiên" (reset 1 lần).
- `std::pmr::monotonic_buffer_resource` + `unsynchronized_pool_resource`: so với pool tự viết.
- Đọc: cách `jemalloc`/`mimalloc`/`tcmalloc` dùng thread-local cache.
