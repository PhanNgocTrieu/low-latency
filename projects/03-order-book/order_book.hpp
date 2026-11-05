// Project 03 — Limit order book + matching engine (price-time priority).
//
// Đây là bản BASELINE: dễ đọc, đúng, nhưng chậm (std::map + std::list + unordered_map
// => mỗi thao tác là nhiều lần pointer chasing và malloc). Nhiệm vụ của bạn: viết
// FastOrderBook cùng interface, qua được cùng bộ test, và nhanh hơn nhiều lần.
#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>

using OrderId = uint64_t;
using Price = int64_t;  // fixed-point: đơn vị là tick, KHÔNG dùng double
using Qty = uint32_t;

enum class Side : uint8_t { Buy, Sell };

struct Trade {
    OrderId maker;  // lệnh đang nằm trên sổ
    OrderId taker;  // lệnh mới đến
    Price price;    // khớp tại giá của maker
    Qty qty;
};

class MapOrderBook {
public:
    // Thêm lệnh limit. Phần khớp được sẽ gọi on_trade cho từng giao dịch; phần còn lại
    // nằm chờ trên sổ. Trả về khối lượng còn lại đã nằm trên sổ (0 nếu khớp hết).
    // Giả định id là duy nhất (bài tập: xử lý id trùng).
    template <class OnTrade>
    Qty add_limit(OrderId id, Side side, Price px, Qty qty, OnTrade&& on_trade) {
        if (side == Side::Buy)
            qty = match(asks_, id, qty, [px](Price lvl) { return lvl <= px; }, on_trade);
        else
            qty = match(bids_, id, qty, [px](Price lvl) { return lvl >= px; }, on_trade);
        if (qty == 0) return 0;
        auto& level = side == Side::Buy ? bids_[px] : asks_[px];
        level.push_back(RestingOrder{id, qty});
        index_.emplace(id, Locator{side, px, std::prev(level.end())});
        return qty;
    }

    // Hủy lệnh. Trả về false nếu id không tồn tại (đã khớp hết hoặc chưa từng có).
    bool cancel(OrderId id) {
        auto it = index_.find(id);
        if (it == index_.end()) return false;
        const Locator loc = it->second;
        index_.erase(it);
        if (loc.side == Side::Buy)
            erase_from(bids_, loc);
        else
            erase_from(asks_, loc);
        return true;
    }

    std::optional<Price> best_bid() const {
        if (bids_.empty()) return std::nullopt;
        return bids_.begin()->first;
    }
    std::optional<Price> best_ask() const {
        if (asks_.empty()) return std::nullopt;
        return asks_.begin()->first;
    }

    // Tổng khối lượng đang chờ tại một mức giá.
    Qty volume_at(Side side, Price px) const {
        const auto sum = [](const auto& book, Price p) -> Qty {
            auto it = book.find(p);
            if (it == book.end()) return 0;
            Qty total = 0;
            for (const auto& o : it->second) total += o.qty;
            return total;
        };
        return side == Side::Buy ? sum(bids_, px) : sum(asks_, px);
    }

    std::size_t order_count() const { return index_.size(); }

private:
    struct RestingOrder {
        OrderId id;
        Qty qty;
    };
    using Level = std::list<RestingOrder>;  // FIFO trong cùng mức giá = time priority
    struct Locator {
        Side side;
        Price px;
        Level::iterator it;
    };

    template <class Book, class Crosses, class OnTrade>
    Qty match(Book& book, OrderId taker, Qty qty, Crosses crosses, OnTrade& on_trade) {
        while (qty > 0 && !book.empty()) {
            auto lvl = book.begin();  // mức giá tốt nhất phía đối diện
            if (!crosses(lvl->first)) break;
            Level& q = lvl->second;
            while (qty > 0 && !q.empty()) {
                RestingOrder& maker = q.front();
                const Qty fill = qty < maker.qty ? qty : maker.qty;
                on_trade(Trade{maker.id, taker, lvl->first, fill});
                qty -= fill;
                maker.qty -= fill;
                if (maker.qty == 0) {
                    index_.erase(maker.id);
                    q.pop_front();
                }
            }
            if (q.empty()) book.erase(lvl);
        }
        return qty;
    }

    template <class Book>
    static void erase_from(Book& book, const Locator& loc) {
        auto lvl = book.find(loc.px);
        lvl->second.erase(loc.it);
        if (lvl->second.empty()) book.erase(lvl);
    }

    std::map<Price, Level, std::greater<Price>> bids_;  // giá cao nhất trước
    std::map<Price, Level, std::less<Price>> asks_;     // giá thấp nhất trước
    std::unordered_map<OrderId, Locator> index_;
};
