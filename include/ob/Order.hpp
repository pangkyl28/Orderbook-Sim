#pragma once
#include <cstdint>

enum class OrderSide {
    BID,
    ASK
};

struct Order{
    uint64_t order_id;
    uint64_t seq; // time basically
    uint64_t quantity;
    uint32_t price_tick;
    OrderSide side;
};


