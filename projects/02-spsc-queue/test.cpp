// Test tính đúng đắn. Khi bạn viết SpscQueueV2/V3, thêm vào danh sách ở main().
// Chạy thêm với ThreadSanitizer: make tsan-02

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "../common/bench.hpp"
#include "mutex_queue.hpp"
#include "spsc_queue.hpp"

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "CHECK failed %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            std::exit(1);                                                        \
        }                                                                        \
    } while (0)

template <class Q>
void test_single_thread(const char* name) {
    Q q(4);
    int v = -1;
    CHECK(!q.try_pop(v));  // rỗng
    for (int i = 0; i < 4; ++i) CHECK(q.try_push(i));
    CHECK(!q.try_push(99));  // đầy
    for (int i = 0; i < 4; ++i) {
        CHECK(q.try_pop(v));
        CHECK(v == i);  // FIFO
    }
    CHECK(!q.try_pop(v));
    // wrap-around nhiều vòng
    for (int round = 0; round < 1000; ++round) {
        CHECK(q.try_push(round));
        CHECK(q.try_push(round + 1));
        CHECK(q.try_pop(v) && v == round);
        CHECK(q.try_pop(v) && v == round + 1);
    }
    std::printf("[PASS] %-14s single-thread\n", name);
}

template <class Q>
void test_two_threads(const char* name, uint64_t n) {
    Q q(1024);
    std::thread producer([&] {
        for (uint64_t i = 0; i < n; ++i)
            while (!q.try_push(i)) ll::cpu_relax();
    });
    uint64_t expected = 0, v = 0;
    while (expected < n) {
        if (q.try_pop(v)) {
            CHECK(v == expected);  // không mất, không trùng, đúng thứ tự
            ++expected;
        } else {
            ll::cpu_relax();
        }
    }
    producer.join();
    CHECK(!q.try_pop(v));
    std::printf("[PASS] %-14s two-threads (%llu items)\n", name, (unsigned long long)n);
}

int main() {
    test_single_thread<SpscQueueV1<int>>("SpscQueueV1");
    test_single_thread<MutexQueue<int>>("MutexQueue");
    test_two_threads<SpscQueueV1<uint64_t>>("SpscQueueV1", 5'000'000);
    test_two_threads<MutexQueue<uint64_t>>("MutexQueue", 1'000'000);
    // TODO: test_single_thread<SpscQueueV2<int>>("SpscQueueV2"); ...
    std::puts("All tests passed.");
}
