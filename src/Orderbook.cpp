#include "ob/OrderBook.hpp"
#include <random>
#include <vector>
#include <algorithm>
#include <sstream>
namespace ob {

    void OrderBook::upsert_by_id(const Order& o) {
        auto& book = (o.side == OrderSide::BID) ? bids : asks;

        for (auto& existing : book) {
            if (existing.order_id == o.order_id) {
                existing = o;   // replace
                return;
            }
        }
        book.push_back(o); // insert
    }

    void OrderBook::erase_by_id(int order_id) {
        auto erase_from = [&](std::vector<Order>& book) {
            book.erase(std::remove_if(book.begin(), book.end(),
                                    [&](const Order& x) { return x.order_id == order_id; }),
                    book.end());
        };
        erase_from(bids);
        erase_from(asks);
    }

    void OrderBook::clear_book() {
        bids.clear();
        asks.clear();
    }

    std::vector<Trade> OrderBook::add_limit(const Order& order) {
        // TODO : implement order matching logic here
        std::vector<Trade> trades_executed;
        uint64_t remaining_qty = order.quantity;

        if (order.side == OrderSide::BID) {
            while (remaining_qty > 0 && !asks.empty() && asks[0].price_tick <= order.price_tick)
            {
                Order& best_ask = asks[0];
                uint64_t trade_qty = std::min(remaining_qty, best_ask.quantity);
                Trade trade = {
                    .trade_id = cur_trade_id++,
                    .price_tick = best_ask.price_tick,
                    .quantity = trade_qty,
                    .taker_order_id = order.order_id, // incoming order is taker
                    .maker_order_id = best_ask.order_id
                };
                trades_executed.push_back(trade);
                remaining_qty -= trade_qty;
                best_ask.quantity -= trade_qty;
                if (asks[0].quantity == 0) {
                    // remove the best ask from the book
                    asks.erase(asks.begin());
                }
            }
            if (remaining_qty > 0)
            {
                Order remaining_order = order;
                remaining_order.quantity = remaining_qty;
                bids.push_back(remaining_order);
                stable_sort(bids.begin(), bids.end(), [](const Order& a, const Order& b){
                    if (a.price_tick != b.price_tick) return a.price_tick > b.price_tick;
                    return a.seq < b.seq; // sort in descending order of price
                });
            }
        }
        else // order.side == OrderSide::ASK
        {
            while (remaining_qty > 0 && !bids.empty() && bids[0].price_tick >= order.price_tick)
            {
                Order& best_bid = bids[0];
                uint64_t trade_qty = std::min(remaining_qty, best_bid.quantity);
                Trade trade = {
                    .trade_id = cur_trade_id++,
                    .price_tick = best_bid.price_tick,
                    .quantity = trade_qty,
                    .taker_order_id = order.order_id, // incoming order is taker
                    .maker_order_id = best_bid.order_id
                };
                trades_executed.push_back(trade);
                remaining_qty -= trade_qty;
                best_bid.quantity -= trade_qty;
                if (bids[0].quantity == 0) {
                    // remove the best bid from the book
                    bids.erase(bids.begin());
                }
            }
            if (remaining_qty > 0)
            {
                Order remaining_order = order;
                remaining_order.quantity = remaining_qty;
                asks.push_back(remaining_order);
                stable_sort(asks.begin(), asks.end(), [](const Order& a, const Order& b){
                    if (a.price_tick != b.price_tick) return a.price_tick < b.price_tick;
                    return a.seq < b.seq; // sort in ascending order of price
                });
            }
        }

        return trades_executed;
    }

    int OrderBook::cancel_order(int order_id) {
        for (size_t i = 0; i < bids.size(); i++)
        {
            if (bids[i].order_id == order_id) {
                bids.erase(bids.begin() + i);
                return 0; // success
            }
        }
        for (size_t i = 0; i < asks.size(); i++)
        {
            if (asks[i].order_id == order_id) {
                asks.erase(asks.begin() + i);
                return 0; // success
            }
        }
        return -1; // order_id not found
    }

    Order* OrderBook::create_order(OrderSide side, uint8_t price_tick, uint16_t quantity) {
        uint64_t id = cur_seq++;
        Order order = {
            .order_id = id,
            .side = side,
            .price_tick = price_tick,
            .quantity = quantity,
            .seq = id
        };
        return new Order(order);
    }



    // ---------- Helper Functions ----------
    size_t OrderBook::bid_count() const {
        return bids.size();
    }
    size_t OrderBook::ask_count() const {
        return asks.size();
    }
    const Order* OrderBook::best_bid() const {
        if (bids.empty()) return nullptr;
        return &bids[0];
    }
    const Order* OrderBook::best_ask() const {
        if (asks.empty()) return nullptr;
        return &asks[0];
    }
    const Order* OrderBook::bid_at(size_t i) const {
        if (i >= bids.size()) return nullptr;
        return &bids[i];
    }
    const Order* OrderBook::ask_at(size_t i) const {
        if (i >= asks.size()) return nullptr;
        return &asks[i];
    }
    std::string OrderBook::dump_book() const {
        std::ostringstream out;

        out << "BIDS\n";
        for (const auto& b : bids)
        {
            out << " id=" << b.order_id
                << " seq=" << b.seq
                << " px=" << static_cast<int>(b.price_tick)
                << " qty=" << b.quantity
                << "\n";
        }
        out << "ASKS\n";
        for (const auto& a : asks)
        {
            out << " id=" << a.order_id
                << " seq=" << a.seq 
                << " px-" << static_cast<int>(a.price_tick)
                << " qty=" << a.quantity
                << "\n";
        }

        return out.str();
    }

    std::string OrderBook::trade_to_string(const Trade& t) const {
        std::ostringstream out;
        out << "trade_id=" << t.trade_id
            << " px=" << t.price_tick
            << " qty=" << t.quantity
            << " taker=" << t.taker_order_id
            << " maker=" << t.maker_order_id;
        return out.str();
    }

    
} // namespace ob