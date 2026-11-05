// Benchmark SPSC queue:
//   (1) Throughput: producer đẩy N phần tử liên tục, consumer lấy ra -> triệu ops/giây.
//   (2) Round-trip latency: ping-pong qua 2 queue, đo từng lượt -> phân phối RTT.
//
// Thêm queue mới của bạn vào main() để so sánh.

#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "../common/bench.hpp"
#include "mutex_queue.hpp"
#include "spsc_queue.hpp"

template <class Q>
void bench_throughput(const char* name, uint64_t n) {
    Q q(4096);
    std::thread consumer([&] {
        ll::pin_to_core(2);
        uint64_t v = 0, sum = 0;
        for (uint64_t i = 0; i < n; ++i) {
            while (!q.try_pop(v)) ll::cpu_relax();
            sum += v;
        }
        ll::do_not_optimize(sum);
    });
    ll::pin_to_core(1);
    const uint64_t t0 = ll::now_ns();
    for (uint64_t i = 0; i < n; ++i)
        while (!q.try_push(i)) ll::cpu_relax();
    consumer.join();
    const double sec = double(ll::now_ns() - t0) / 1e9;
    std::printf("%-28s throughput: %8.2f M ops/s\n", name, n / sec / 1e6);
}

template <class Q>
void bench_rtt(const char* name, uint64_t iters) {
    Q ping(1024), pong(1024);
    std::thread echo([&] {
        ll::pin_to_core(2);
        uint64_t v = 0;
        for (uint64_t i = 0; i < iters; ++i) {
            while (!ping.try_pop(v)) ll::cpu_relax();
            while (!pong.try_push(v)) ll::cpu_relax();
        }
    });
    ll::pin_to_core(1);
    std::vector<uint64_t> samples;
    samples.reserve(iters);
    uint64_t v = 0;
    for (uint64_t i = 0; i < iters; ++i) {
        const uint64_t t0 = ll::now_ns();
        while (!ping.try_push(i)) ll::cpu_relax();
        while (!pong.try_pop(v)) ll::cpu_relax();
        samples.push_back(ll::now_ns() - t0);
    }
    echo.join();
    // Bỏ 10% mẫu đầu (warm-up)
    samples.erase(samples.begin(), samples.begin() + iters / 10);
    std::string label = std::string(name) + " RTT";
    ll::report_latency(label.c_str(), samples);
}

int main() {
    std::puts("== Throughput ==");
    bench_throughput<MutexQueue<uint64_t>>("MutexQueue", 10'000'000);
    bench_throughput<SpscQueueV1<uint64_t>>("SpscQueueV1", 50'000'000);
    // TODO: bench_throughput<SpscQueueV2<uint64_t>>("SpscQueueV2", 50'000'000);

    std::puts("\n== Round-trip latency (ping-pong giữa 2 thread) ==");
    bench_rtt<MutexQueue<uint64_t>>("MutexQueue", 1'000'000);
    bench_rtt<SpscQueueV1<uint64_t>>("SpscQueueV1", 1'000'000);
    // TODO: bench_rtt<SpscQueueV2<uint64_t>>("SpscQueueV2", 1'000'000);
}
