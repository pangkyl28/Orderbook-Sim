#pragma once
#include <cstdint>

struct Trade{
    int trade_id;
    int price_tick;
    int64_t quantity;
    int taker_order_id; // id of the order that initiated the trade
    int maker_order_id; // id of the order that was resting in the book
};

// Trades (trade_id, price, qty, taker_order_id, maker_order_id)