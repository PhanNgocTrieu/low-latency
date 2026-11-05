// Test hành vi order book. Khi viết FastOrderBook, đổi alias Book (hoặc chạy cả hai)
// để đảm bảo bản tối ưu cho kết quả GIỐNG HỆT baseline.

#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

#include "order_book.hpp"

#define CHECK(cond)                                                                      \
    do {                                                                                 \
        if (!(cond)) {                                                                   \
            std::fprintf(stderr, "CHECK failed %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            std::exit(1);                                                                \
        }                                                                                \
    } while (0)

template <class Book>
struct Tests {
    static void resting_and_bbo() {
        Book b;
        std::vector<Trade> trades;
        auto rec = [&](const Trade& t) { trades.push_back(t); };
        CHECK(!b.best_bid() && !b.best_ask());
        CHECK(b.add_limit(1, Side::Buy, 100, 10, rec) == 10);
        CHECK(b.add_limit(2, Side::Buy, 101, 5, rec) == 5);
        CHECK(b.add_limit(3, Side::Sell, 105, 7, rec) == 7);
        CHECK(b.add_limit(4, Side::Sell, 103, 3, rec) == 3);
        CHECK(trades.empty());
        CHECK(*b.best_bid() == 101);
        CHECK(*b.best_ask() == 103);
        CHECK(b.order_count() == 4);
        CHECK(b.volume_at(Side::Buy, 100) == 10);
    }

    static void price_time_priority() {
        Book b;
        std::vector<Trade> trades;
        auto rec = [&](const Trade& t) { trades.push_back(t); };
        b.add_limit(1, Side::Sell, 100, 5, rec);
        b.add_limit(2, Side::Sell, 100, 5, rec);  // cùng giá, đến sau
        b.add_limit(3, Side::Sell, 99, 5, rec);   // giá tốt hơn
        // Lệnh mua 12 @ 100: khớp 99 trước (giá), rồi id1, rồi id2 (thời gian)
        CHECK(b.add_limit(10, Side::Buy, 100, 12, rec) == 0);
        CHECK(trades.size() == 3);
        CHECK(trades[0].maker == 3 && trades[0].price == 99 && trades[0].qty == 5);
        CHECK(trades[1].maker == 1 && trades[1].price == 100 && trades[1].qty == 5);
        CHECK(trades[2].maker == 2 && trades[2].price == 100 && trades[2].qty == 2);
        CHECK(b.volume_at(Side::Sell, 100) == 3);
        CHECK(b.order_count() == 1);
    }

    static void partial_fill_rests() {
        Book b;
        std::vector<Trade> trades;
        auto rec = [&](const Trade& t) { trades.push_back(t); };
        b.add_limit(1, Side::Buy, 50, 4, rec);
        CHECK(b.add_limit(2, Side::Sell, 49, 10, rec) == 6);  // khớp 4, nằm lại 6 @ 49
        CHECK(trades.size() == 1 && trades[0].price == 50);   // khớp tại giá maker
        CHECK(!b.best_bid());
        CHECK(*b.best_ask() == 49);
    }

    static void cancel_works() {
        Book b;
        auto rec = [](const Trade&) {};
        b.add_limit(1, Side::Buy, 100, 10, rec);
        b.add_limit(2, Side::Buy, 100, 20, rec);
        CHECK(b.cancel(1));
        CHECK(!b.cancel(1));
        CHECK(!b.cancel(999));
        CHECK(b.volume_at(Side::Buy, 100) == 20);
        CHECK(b.cancel(2));
        CHECK(!b.best_bid());
        CHECK(b.order_count() == 0);
    }

    static void run(const char* name) {
        resting_and_bbo();
        price_time_priority();
        partial_fill_rests();
        cancel_works();
        std::printf("[PASS] %s\n", name);
    }
};

// So sánh 2 cài đặt trên chuỗi lệnh ngẫu nhiên: mọi trade và BBO phải trùng khớp.
template <class A, class B>
void differential_test(uint64_t ops) {
    A a;
    B b;
    std::vector<Trade> ta, tb;
    std::mt19937_64 rng(123);
    std::vector<OrderId> live;
    for (uint64_t i = 1; i <= ops; ++i) {
        if (!live.empty() && rng() % 3 == 0) {
            std::size_t k = rng() % live.size();
            CHECK(a.cancel(live[k]) == b.cancel(live[k]));
            live[k] = live.back();
            live.pop_back();
        } else {
            Side s = rng() % 2 ? Side::Buy : Side::Sell;
            Price px = 1000 + Price(rng() % 40) - 20;
            Qty q = 1 + Qty(rng() % 100);
            Qty ra = a.add_limit(i, s, px, q, [&](const Trade& t) { ta.push_back(t); });
            Qty rb = b.add_limit(i, s, px, q, [&](const Trade& t) { tb.push_back(t); });
            CHECK(ra == rb);
            if (ra > 0) live.push_back(i);
        }
        CHECK(a.best_bid() == b.best_bid());
        CHECK(a.best_ask() == b.best_ask());
    }
    CHECK(ta.size() == tb.size());
    for (std::size_t i = 0; i < ta.size(); ++i)
        CHECK(ta[i].maker == tb[i].maker && ta[i].taker == tb[i].taker &&
              ta[i].price == tb[i].price && ta[i].qty == tb[i].qty);
    std::printf("[PASS] differential test (%llu ops, %zu trades)\n", (unsigned long long)ops,
                ta.size());
}

int main() {
    Tests<MapOrderBook>::run("MapOrderBook");
    // Lưu ý: list `live` có thể chứa id đã bị khớp hết -> cancel trả false ở cả hai bên, vẫn hợp lệ.
    differential_test<MapOrderBook, MapOrderBook>(200'000);
    // TODO: Tests<FastOrderBook>::run("FastOrderBook");
    // TODO: differential_test<MapOrderBook, FastOrderBook>(2'000'000);
    std::puts("All tests passed.");
}
