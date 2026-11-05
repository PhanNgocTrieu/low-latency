// Project 01 — Tự đo các hiệu ứng phần cứng quan trọng nhất cho low latency.
//
//   1. Memory hierarchy: latency truy cập ngẫu nhiên theo kích thước working set (pointer chasing)
//   2. Branch prediction: dữ liệu ngẫu nhiên vs đã sắp xếp
//   3. False sharing: 2 thread ghi 2 biến nằm chung / khác cache line
//
// Chạy: make run-01   (hoặc ./build/01-hardware-effects [cache|branch|sharing])

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "../common/bench.hpp"

// ---------------------------------------------------------------------------
// 1. Memory hierarchy — pointer chasing
// Mỗi node chiếm trọn 1 cache line; các node nối với nhau theo hoán vị ngẫu nhiên
// => prefetcher không đoán được, mỗi bước là 1 lần truy cập phụ thuộc vào bước trước.
// Thời gian / bước ≈ latency thật của tầng bộ nhớ chứa được working set.
// ---------------------------------------------------------------------------
struct alignas(ll::kCacheLine) Node {
    Node* next;
};

static double chase_ns_per_hop(std::size_t bytes, std::mt19937_64& rng) {
    const std::size_t n = std::max<std::size_t>(bytes / sizeof(Node), 2);
    std::vector<Node> nodes(n);
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng);
    for (std::size_t i = 0; i < n; ++i) nodes[order[i]].next = &nodes[order[(i + 1) % n]];

    Node* p = &nodes[order[0]];
    for (std::size_t i = 0; i < n; ++i) p = p->next;  // warm-up: chạm hết các trang

    constexpr std::size_t kHops = 10'000'000;
    const uint64_t t0 = ll::now_ns();
    for (std::size_t i = 0; i < kHops; ++i) p = p->next;
    const uint64_t t1 = ll::now_ns();
    ll::do_not_optimize(p);
    return double(t1 - t0) / kHops;
}

static void run_cache() {
    std::puts("\n== 1. Memory hierarchy: random pointer chasing ==");
    std::puts("working set      ns/hop   (quan sát các 'bậc thang': L1 -> L2 -> L3/SLC -> DRAM)");
    std::mt19937_64 rng(42);
    for (std::size_t kb = 16; kb <= 256 * 1024; kb *= 2) {
        double ns = chase_ns_per_hop(kb * 1024, rng);
        std::printf("%8zu KB   %8.2f   ", kb, ns);
        int bar = std::min(60, int(ns));
        for (int i = 0; i < bar; ++i) std::putchar('#');
        std::putchar('\n');
    }
}

// ---------------------------------------------------------------------------
// 2. Branch prediction
// asm volatile("") bên trong nhánh ngăn compiler biến if thành lệnh branchless (csel/cmov)
// hay vector hóa — để ta thấy được chi phí thật của branch mispredict.
// Bài tập: bỏ dòng asm đó, compile lại, xem assembly trên godbolt và đo lại.
// ---------------------------------------------------------------------------
static uint64_t sum_branchy(const std::vector<uint8_t>& data) {
    uint64_t sum = 0;
    for (uint8_t v : data) {
        if (v >= 128) {
            sum += v;
            asm volatile("");
        }
    }
    return sum;
}

static uint64_t sum_branchless(const std::vector<uint8_t>& data) {
    uint64_t sum = 0;
    for (uint8_t v : data) {
        uint64_t mask = -uint64_t(v >= 128);  // 0xFFFF... nếu đúng, 0 nếu sai
        sum += v & mask;
    }
    return sum;
}

template <class F>
static double time_ns_per_elem(F&& f, const std::vector<uint8_t>& data, int reps = 10) {
    uint64_t best = UINT64_MAX;
    for (int r = 0; r < reps; ++r) {
        uint64_t t0 = ll::now_ns();
        ll::do_not_optimize(f(data));
        best = std::min(best, ll::now_ns() - t0);
    }
    return double(best) / data.size();
}

static void run_branch() {
    std::puts("\n== 2. Branch prediction ==");
    std::vector<uint8_t> data(16 * 1024 * 1024);
    std::mt19937 rng(7);
    for (auto& v : data) v = uint8_t(rng());
    auto sorted = data;
    std::sort(sorted.begin(), sorted.end());

    std::printf("branchy,    unsorted : %.3f ns/elem  <- ~50%% đoán sai\n",
                time_ns_per_elem(sum_branchy, data));
    std::printf("branchy,    sorted   : %.3f ns/elem  <- predictor đoán gần như đúng hết\n",
                time_ns_per_elem(sum_branchy, sorted));
    std::printf("branchless, unsorted : %.3f ns/elem  <- không phụ thuộc dữ liệu\n",
                time_ns_per_elem(sum_branchless, data));
    std::printf("branchless, sorted   : %.3f ns/elem\n", time_ns_per_elem(sum_branchless, sorted));
}

// ---------------------------------------------------------------------------
// 3. False sharing
// ---------------------------------------------------------------------------
struct Packed {
    std::atomic<uint64_t> a{0};
    std::atomic<uint64_t> b{0};  // cùng cache line với a
};
struct Padded {
    alignas(ll::kCacheLine) std::atomic<uint64_t> a{0};
    alignas(ll::kCacheLine) std::atomic<uint64_t> b{0};  // line riêng
};

template <class S>
static double run_two_writers(S& s) {
    constexpr uint64_t kIters = 50'000'000;
    std::atomic<bool> go{false};
    auto worker = [&](std::atomic<uint64_t>& c, int core) {
        ll::pin_to_core(core);
        while (!go.load(std::memory_order_acquire)) ll::cpu_relax();
        for (uint64_t i = 0; i < kIters; ++i) c.fetch_add(1, std::memory_order_relaxed);
    };
    std::thread t1(worker, std::ref(s.a), 1);
    std::thread t2(worker, std::ref(s.b), 2);
    uint64_t t0 = ll::now_ns();
    go.store(true, std::memory_order_release);
    t1.join();
    t2.join();
    return double(ll::now_ns() - t0) / 1e6;
}

static void run_sharing() {
    std::puts("\n== 3. False sharing (2 thread, mỗi thread 50M lần fetch_add vào biến riêng) ==");
    Packed packed;
    Padded padded;
    std::printf("sizeof(Packed)=%zu  sizeof(Padded)=%zu\n", sizeof(Packed), sizeof(Padded));
    std::printf("cùng cache line : %8.1f ms\n", run_two_writers(packed));
    std::printf("khác cache line : %8.1f ms\n", run_two_writers(padded));
}

int main(int argc, char** argv) {
    std::string which = argc > 1 ? argv[1] : "all";
    if (which == "all" || which == "cache") run_cache();
    if (which == "all" || which == "branch") run_branch();
    if (which == "all" || which == "sharing") run_sharing();
    return 0;
}
