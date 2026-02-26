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

        private:
            // helper functions and other stuff for order matching and trade execution
            uint64_t cur_trade_id = 0;
            uint64_t cur_seq = 0;

            // for now use a vector to store orders, will need a more efficient structure for real implementation
            std::vector<Order> bids;
            std::vector<Order> asks;
    };

}