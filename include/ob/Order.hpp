#pragma once
#include <cstdint>

enum class OrderSide {
    BID,
    ASK
};

struct Order{
    int order_id;
    OrderSide side;
    int price_tick; // i think in prediction markets we should be using this instead of price
    // ^ if we're working in kalshi
    int64_t quantity;
    int seq; // time basically
};


