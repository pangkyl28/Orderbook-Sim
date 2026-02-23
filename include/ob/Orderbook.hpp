#pragma once

#include "Order.hpp"
#include "Trade.hpp"
#include <vector>
#include <algorithm>

namespace ob {
    class OrderBook {
        public:
            // should return the list of trades executed as a result of adding the order
            std::vector<Trade> add_limit(const Order& order); 

            // should cancel the order with given order_id
            // return 0 on success, -1 if order_id not found
            int cancel_order(int order_id); // -> Implement later

            // adds a market order to OB -> executes trades immediately
            // returns 0 on success, -1 on failure
            // std::vector<Trade> add_market(const Order& order); // -> Implement later
            // need a way to represent market orders


            const std::vector<Order>& get_bids() const { return bids; }
            const std::vector<Order>& get_asks() const { return asks; }
            Order* create_order(OrderSide side, int price_tick, int quantity);
            
            void upsert_by_id(const Order& o);
            void erase_by_id(int order_id);
            void clear_book();


        private:
            // helper functions and other stuff for order matching and trade execution
            int cur_trade_id = 0;
            int cur_seq = 0;

            // for now use a vector to store orders, will need a more efficient structure for real implementation
            std::vector<Order> bids;
            std::vector<Order> asks;
    };

}