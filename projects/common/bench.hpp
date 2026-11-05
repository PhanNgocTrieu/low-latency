// Tiện ích benchmark dùng chung cho mọi project.
#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace ll {

// Thời gian monotonic tính bằng nanosecond.
// macOS: steady_clock dựa trên mach_absolute_time, độ phân giải ~41.7 ns trên Apple Silicon.
// Linux: clock_gettime(CLOCK_MONOTONIC) qua vDSO, ~20 ns overhead.
inline uint64_t now_ns() noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     std::chrono::steady_clock::now().time_since_epoch())
                                     .count());
}

// Bộ đếm chu kỳ / tick phần cứng (rẻ hơn now_ns). Cần hiệu chuẩn nếu muốn đổi ra ns.
inline uint64_t ticks() noexcept {
#if defined(__x86_64__)
    uint32_t lo, hi;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi)::"rcx");
    return (static_cast<uint64_t>(hi) << 32) | lo;
#elif defined(__aarch64__)
    uint64_t v;
    asm volatile("isb; mrs %0, cntvct_el0" : "=r"(v)::"memory");
    return v;
#else
    return now_ns();
#endif
}

// Ngăn compiler xóa phép tính có kết quả không được dùng (giống benchmark::DoNotOptimize).
template <class T>
inline void do_not_optimize(T const& value) noexcept {
    asm volatile("" : : "r,m"(value) : "memory");
}

// Ngăn compiler gộp/sắp xếp lại các thao tác bộ nhớ qua điểm này.
inline void clobber_memory() noexcept { asm volatile("" : : : "memory"); }

// Gợi ý CPU rằng ta đang spin-wait (giảm tiêu thụ tài nguyên của hyper-thread anh em).
inline void cpu_relax() noexcept {
#if defined(__x86_64__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    asm volatile("yield" ::: "memory");
#endif
}

// Pin thread hiện tại vào 1 core. Chỉ thực sự có tác dụng trên Linux.
// macOS không hỗ trợ CPU affinity cứng -> hàm này không làm gì, kết quả sẽ nhiễu hơn.
inline bool pin_to_core([[maybe_unused]] int core) noexcept {
#if defined(__linux__)
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
#else
    return false;
#endif
}

// Kích thước dùng để tránh false sharing.
// x86: 64 byte, nhưng adjacent-line prefetcher kéo theo cặp line -> 128 an toàn hơn.
// Apple M-series: cache line 128 byte.
inline constexpr std::size_t kCacheLine = 128;

// In phân phối latency: min / p50 / p90 / p99 / p99.9 / p99.99 / max / mean.
// Cách đơn giản: sort toàn bộ mẫu. (Bài tập module 05: thay bằng HDR histogram.)
inline void report_latency(const char* name, std::vector<uint64_t>& samples_ns) {
    if (samples_ns.empty()) return;
    std::sort(samples_ns.begin(), samples_ns.end());
    auto pct = [&](double p) {
        std::size_t idx = static_cast<std::size_t>(p / 100.0 * (samples_ns.size() - 1));
        return samples_ns[idx];
    };
    long double sum = 0;
    for (auto v : samples_ns) sum += v;
    std::printf(
        "%-28s n=%-9zu min=%-6llu p50=%-6llu p90=%-6llu p99=%-6llu p99.9=%-7llu p99.99=%-7llu "
        "max=%-8llu mean=%.1f (ns)\n",
        name, samples_ns.size(), (unsigned long long)samples_ns.front(),
        (unsigned long long)pct(50), (unsigned long long)pct(90), (unsigned long long)pct(99),
        (unsigned long long)pct(99.9), (unsigned long long)pct(99.99),
        (unsigned long long)samples_ns.back(), (double)(sum / samples_ns.size()));
}

}  // namespace ll
