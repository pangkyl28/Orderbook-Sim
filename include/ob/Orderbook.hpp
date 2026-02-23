#pragma once

using namespace std;

#include "Order.hpp"
#include "Trade.hpp"
#include <vector>
#include <algorithm>

namespace ob {
    class OrderBook {
        public:
            // should return the list of trades executed as a result of adding the order
            vector<Trade> add_limit(const Order& order); 

            // should cancel the order with given order_id
            // return 0 on success, -1 if order_id not found
            int cancel_order(int order_id); // -> Implement later

            // adds a market order to OB -> executes trades immediately
            // returns 0 on success, -1 on failure
            // std::vector<Trade> add_market(const Order& order); // -> Implement later
            // need a way to represent market orders


        private:
            // helper functions and other stuff for order matching and trade execution
            int cur_trade_id = 0;
            int cur_seq = 0;

            // for now use a vector to store orders, will need a more efficient structure for real implementation
            vector<Order> bids;
            vector<Order> asks;
    };

}