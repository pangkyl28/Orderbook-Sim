#pragma once
#include <stdint.h>

enum class OrderSide {
    BID,
    ASK
};

struct Order{
    uint64_t order_id;
    uint64_t seq; // time basically
    uint16_t quantity;
    uint8_t price_tick;
    OrderSide side;
};