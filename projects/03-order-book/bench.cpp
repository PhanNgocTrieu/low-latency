// Benchmark order book với luồng lệnh tổng hợp giống thực tế:
//  - Giá tập trung quanh mid (phân phối gần chuẩn), mid trôi chậm (random walk)
//  - ~55% add, ~40% cancel, phần còn lại là lệnh aggressive (khớp ngay)
// Dữ liệu sinh TRƯỚC khi đo để không đo nhầm chi phí của random generator.
//
// Lưu ý macOS: độ phân giải đồng hồ ~41.7 ns, nên các mẫu nhỏ sẽ bị "lượng tử hóa".
// Hãy nhìn cả throughput (ops/s) lẫn percentile.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

#include "../common/bench.hpp"
#include "order_book.hpp"

struct Op {
    enum Kind : uint8_t { Add, Cancel } kind;
    Side side;
    OrderId id;
    Price px;
    Qty qty;
};

static std::vector<Op> generate(std::size_t n) {
    std::vector<Op> ops;
    ops.reserve(n);
    std::mt19937_64 rng(2024);
    std::normal_distribution<double> offset(0.0, 8.0);
    std::vector<OrderId> live;
    Price mid = 10'000;
    OrderId next_id = 1;
    for (std::size_t i = 0; i < n; ++i) {
        if (i % 1000 == 0) mid += Price(rng() % 3) - 1;
        const uint64_t r = rng() % 100;
        if (r < 40 && !live.empty()) {
            std::size_t k = rng() % live.size();
            ops.push_back({Op::Cancel, Side::Buy, live[k], 0, 0});
            live[k] = live.back();
            live.pop_back();
        } else {
            Side s = rng() % 2 ? Side::Buy : Side::Sell;
            Price d = Price(std::abs(offset(rng))) + 1;
            Price px = s == Side::Buy ? mid - d : mid + d;
            if (r >= 95) px = s == Side::Buy ? mid + 5 : mid - 5;  // aggressive
            ops.push_back({Op::Add, s, next_id, px, Qty(1 + rng() % 200)});
            live.push_back(next_id++);
        }
    }
    return ops;
}

template <class Book>
void run(const char* name, const std::vector<Op>& ops) {
    Book book;
    uint64_t trades = 0;
    auto on_trade = [&](const Trade&) { ++trades; };
    std::vector<uint64_t> samples;
    samples.reserve(ops.size());

    const uint64_t start = ll::now_ns();
    for (const Op& op : ops) {
        const uint64_t t0 = ll::now_ns();
        if (op.kind == Op::Add)
            book.add_limit(op.id, op.side, op.px, op.qty, on_trade);
        else
            book.cancel(op.id);
        samples.push_back(ll::now_ns() - t0);
    }
    const double sec = double(ll::now_ns() - start) / 1e9;

    std::printf("%s: %zu ops, %llu trades, %zu resting, %.2f M ops/s (gồm overhead đo)\n", name,
                ops.size(), (unsigned long long)trades, book.order_count(),
                ops.size() / sec / 1e6);
    ll::report_latency(name, samples);
}

int main() {
    const auto ops = generate(5'000'000);
    run<MapOrderBook>("MapOrderBook", ops);
    // TODO: run<FastOrderBook>("FastOrderBook", ops);
}
