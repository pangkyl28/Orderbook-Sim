#include "ob/OrderBook.hpp"
#include <random>
#include <vector>
#include <algorithm>
#include <sstream>
namespace ob {

    void OrderBook::upsert_by_id(const Order& o) {
        auto found = order_index_.find(o.order_id);
        if (found != order_index_.end()) {
            OrderRef ref = found->second;

            // case 1: same side and same price level -> update in place
            if (ref.side == o.side && ref.price_tick == o.price_tick) {
                Order updated = o;
                if (updated.seq == 0) {
                    updated.seq = ref.it->seq;
                }

                *(ref.it) = updated;
                order_index_[o.order_id] = OrderRef{
                    .side = updated.side,
                    .price_tick = updated.price_tick,
                    .it = ref.it
                };
                return;
            }

            // case 2: existing order moved -> erase old copy first
            if (ref.side == OrderSide::BID) {
                auto level_it = bids_.find(ref.price_tick);
                if (level_it != bids_.end()) {
                    level_it->second.erase(ref.it);
                    if (level_it->second.empty()) {
                        bids_.erase(level_it);
                    }
                }
            } else { // OrderSide::ASK
                auto level_it = asks_.find(ref.price_tick);
                if (level_it != asks_.end()) {
                    level_it->second.erase(ref.it);
                    if (level_it->second.empty()) {
                        asks_.erase(level_it);
                    }
                }
            }
            order_index_.erase(found);
        }


        if (o.side == OrderSide::BID) {
            BidLevels& book = bids_;

            // find/create price level
            auto level_it = book.find(o.price_tick);
            if (level_it == book.end()) {
                level_it = book.emplace(o.price_tick, std::list<Order>{}).first;
            }

            auto& level_list = level_it->second;
            // append new order
            level_list.push_back(o);
            auto it = std::prev(level_list.end());
            order_index_[o.order_id] = OrderRef{
                .side = o.side,
                .price_tick = o.price_tick,
                .it = it
            };
        }
        else {
            AskLevels& book = asks_;

            // find/create price level
            auto level_it = book.find(o.price_tick);
            if (level_it == book.end()) {
                level_it = book.emplace(o.price_tick, std::list<Order>{}).first;
            }

            auto& level_list = level_it->second;
            // append new order
            level_list.push_back(o);
            auto it = std::prev(level_list.end());
            order_index_[o.order_id] = OrderRef{
                .side = o.side,
                .price_tick = o.price_tick,
                .it = it
            }; 
        }
    }

    void OrderBook::clear_book() {
        bids_.clear();
        asks_.clear();
        order_index_.clear();
    }

    std::vector<Trade> OrderBook::add_limit(const Order& order) {
        // TODO : implement order matching logic here
        std::vector<Trade> trades_executed;
        uint64_t remaining_qty = order.quantity;

        if (order.side == OrderSide::BID) {
            while (remaining_qty > 0 && !asks_.empty() && asks_.begin()->first <= order.price_tick)
            {
                auto level_it = asks_.begin();
                auto& level_list = level_it->second;
                Order& best_ask = level_list.front();

                uint64_t trade_qty = std::min(remaining_qty, best_ask.quantity);
                Trade trade = {
                    .trade_id = ++cur_trade_id,
                    .price_tick = best_ask.price_tick,
                    .quantity = trade_qty,
                    .taker_order_id = order.order_id, // incoming order is taker
                    .maker_order_id = best_ask.order_id
                };

                trades_executed.push_back(trade);

                remaining_qty -= trade_qty;
                best_ask.quantity -= trade_qty;

                if (best_ask.quantity == 0) {
                    // remove the best ask from the book
                    order_index_.erase(best_ask.order_id);
                    level_list.pop_front();

                    if (level_list.empty()) {
                        asks_.erase(level_it);
                    }
                }
            }
            if (remaining_qty > 0)
            {
                Order remaining_order = order;
                remaining_order.quantity = remaining_qty;
                if (remaining_order.seq == 0) { // if order does not currently have a seq, give it one
                    remaining_order.seq = ++cur_seq;
                }
                upsert_by_id(remaining_order);
            }
        }
        else // order.side == OrderSide::ASK
        {
            while (remaining_qty > 0 && !bids_.empty() && bids_.begin()->first >= order.price_tick)
            {
                auto level_it = bids_.begin();
                auto& level_list = level_it->second;
                Order& best_bid = level_list.front();

                uint64_t trade_qty = std::min(remaining_qty, best_bid.quantity);

                Trade trade = {
                    .trade_id = ++cur_trade_id,
                    .price_tick = best_bid.price_tick,
                    .quantity = trade_qty,
                    .taker_order_id = order.order_id, // incoming order is taker
                    .maker_order_id = best_bid.order_id
                };

                trades_executed.push_back(trade);

                remaining_qty -= trade_qty;
                best_bid.quantity -= trade_qty;

                if (best_bid.quantity == 0) {
                    // remove the best bid from the book
                    order_index_.erase(best_bid.order_id);
                    level_list.pop_front();

                    if (level_list.empty()) {
                        bids_.erase(level_it);
                    }
                }
            }
            if (remaining_qty > 0)
            {
                Order remaining_order = order;
                remaining_order.quantity = remaining_qty;
                if (remaining_order.seq == 0) {
                    remaining_order.seq = ++cur_seq;
                }
                upsert_by_id(remaining_order);
            }
        }

        return trades_executed;
    }

    int OrderBook::cancel_order(std::uint64_t order_id) {
        auto ref_it = order_index_.find(order_id);
        if (ref_it == order_index_.end()){ return -1;}

        OrderRef ref = ref_it->second;

        if (ref.side == OrderSide::BID) {
            auto level_it = bids_.find(ref.price_tick);
            if (level_it ==bids_.end()) {return -1;}
            level_it->second.erase(ref.it);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        } else { // OrderSide::ASK
            auto level_it = asks_.find(ref.price_tick);
            if (level_it == asks_.end()) {return -1;}
            level_it->second.erase(ref.it);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }

        order_index_.erase(ref_it);
        return 0;
    }

    Order* OrderBook::create_order(OrderSide side, Tick price_tick, std::uint64_t quantity) {
        Order* order = new Order;
        order->seq = ++cur_seq;
        order->order_id = order->seq;
        order->quantity = quantity;
        order->price_tick = price_tick;
        order->side = side;
        return order;
    }


    

    // ---------- Helper Functions ----------
    size_t OrderBook::bid_count() const {
        size_t count = 0;
        for (const auto& [price, level_list] : bids_) {
            count += level_list.size();
        }
        return count;
    }

    size_t OrderBook::ask_count() const {
        size_t count = 0;
        for (const auto& [price, level_list] : asks_) {
            count += level_list.size();
        }
        return count;
    }

    const Order* OrderBook::best_bid() const {
        if (bids_.empty()) {
            return nullptr;
        }
        const auto& level_list = bids_.begin()->second;
        if (level_list.empty()) {
            return nullptr;
        }
        return &level_list.front();
    }

    const Order* OrderBook::best_ask() const {
        if (asks_.empty()) {
            return nullptr;
        }
        const auto& level_list = asks_.begin()->second;
        if (level_list.empty()) {
            return nullptr;
        }
        return &level_list.front();
    }

    const Order* OrderBook::bid_at(size_t i) const {
        size_t idx = 0;

        for (const auto& [price, level_list] : bids_) {
            for (const auto& order : level_list) {
                if (idx == i) {
                    return &order;
                }
                idx++;
            }
        }
        return nullptr;
    }

    const Order* OrderBook::ask_at(size_t i) const {
        size_t idx = 0;

        for (const auto& [price, level_list] : asks_) {
            for (const auto& order : level_list) {
                if (idx == i) {
                    return &order;
                }
                idx++;
            }
        }
        return nullptr;
    }
        
    std::string OrderBook::dump_book() const {
        std::ostringstream out;

        out << "BIDS:\n";
        for (const auto& [price, level_list] : bids_) {
            out << "Price " << price << ":\n";
            for (const auto& order : level_list) {
                out << "    id=" << order.order_id
                    << " seq=" << order.seq
                    << " qty=" << order.quantity
                    << " px=" << order.price_tick
                    << '\n';
            }
        }

        out << "ASKS:\n";
        for (const auto& [price, level_list] : asks_) {
            out << "Price " << price << ":\n";
            for (const auto& order : level_list) {
                out << "    id=" << order.order_id
                    << " seq=" << order.seq
                    << " qty=" << order.quantity
                    << " px=" << order.price_tick
                    << '\n';
            }
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