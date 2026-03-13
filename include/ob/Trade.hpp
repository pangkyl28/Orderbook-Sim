#pragma once
#include <cstdint>

struct Trade{
    uint64_t trade_id;
    uint64_t taker_order_id; // id of the order that initiated the trade
    uint64_t maker_order_id; // id of the order that was resting in the book
    uint64_t quantity;
    uint32_t price_tick;
};