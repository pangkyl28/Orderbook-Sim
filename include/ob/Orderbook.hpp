#pragma once

#include "Order.hpp"
#include "Trade.hpp"
#include <vector>
#include <stdint.h>

namespace ob {
    class OrderBook {
        public:
            // should return the list of trades executed as a result of adding the order
            std::vector<Trade> add_limit(const Order& order); 

            // should cancel the order with given order_id
            // return 0 on success, -1 if order_id not found
            int cancel_order(int order_id); // -> Implement later

            const std::vector<Order>& get_bids() const { return bids; }
            const std::vector<Order>& get_asks() const { return asks; }
            Order* create_order(OrderSide side, uint8_t price_tick, uint16_t quantity);
            
            void upsert_by_id(const Order& o);
            void erase_by_id(int order_id);
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
            std::vector<Order> bids;
            std::vector<Order> asks;
    };

} // namespace ob