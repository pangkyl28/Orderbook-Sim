#pragma once

#include "Order.hpp"
#include "Trade.hpp"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace ob {

    using Tick = std::uint32_t;
    class OrderBook {
        public:
            // should return the list of trades executed as a result of adding the order
            std::vector<Trade> add_limit(const Order& order); 

            // should cancel the order with given order_id
            // return 0 on success, -1 if order_id not found
            int cancel_order(std::uint64_t order_id);

            // const std::vector<Order>& get_bids() const { return bids; }
            // const std::vector<Order>& get_asks() const { return asks; }
            Order* create_order(OrderSide side, Tick price_tick, std::uint64_t quantity);
            
            void upsert_by_id(const Order& o);
            void clear_book();

            // ---------- Helper Functions ----------
            size_t bid_count() const;
            size_t ask_count() const;

            const Order* best_bid() const;
            const Order* best_ask() const;

            const Order* bid_at(size_t i) const;
            const Order* ask_at(size_t i) const;

            std::string dump_book() const;
            std::string trade_to_string(const Trade& t) const;


        private:
            // helper functions and other stuff for order matching and trade execution
            uint64_t cur_trade_id = 0;
            uint64_t cur_seq = 0;

            // for now use a vector to store orders, will need a more efficient structure for real implementation
            // std::vector<Order> bids;
            // std::vector<Order> asks;

            using BidLevels = std::map<Tick, std::list<Order>, std::greater<Tick>>;
            using AskLevels = std::map<Tick, std::list<Order>, std::less<Tick>>;
            struct OrderRef {
                OrderSide side;
                Tick price_tick;
                std::list<Order>::iterator it;
            };

            BidLevels bids_;
            AskLevels asks_;
            // order_id -> position of order in price-level list
            std::unordered_map<std::uint64_t, OrderRef> order_index_;

    };

} // namespace ob