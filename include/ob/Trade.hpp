#pragma once
#include <stdint.h>

struct Trade{
    uint64_t trade_id;
    int price_tick;
    uint16_t quantity;
    uint64_t taker_order_id; // id of the order that initiated the trade
    uint64_t maker_order_id; // id of the order that was resting in the book
};

// Trades (trade_id, price, qty, taker_order_id, maker_order_id)