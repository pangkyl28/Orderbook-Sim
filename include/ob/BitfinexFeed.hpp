#pragma once
#include <string>

namespace ob {
class OrderBook;

class BitfinexFeed {
public:
    explicit BitfinexFeed(OrderBook& book);
    void run(const std::string& pair = "tBTCUSD"); // blocks forever

private:
    OrderBook& book_;
};
} // namespace ob