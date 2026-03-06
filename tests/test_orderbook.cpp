#include <gtest/gtest.h>
#include <vector>

#include "ob/OrderBook.hpp"
#include "ob/Order.hpp"
#include "ob/Trade.hpp"

using namespace ob;

TEST(OrderBookTest, AddSingleBidRestsOnBook) {
    OrderBook book;

    Order bid{
        .order_id = 1,
        .seq = 1,
        .quantity = 10,
        .price_tick = 100,
        .side = OrderSide::BID
    };

    std::vector<Trade> trades = book.add_limit(bid);

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.bid_count(), 1);
    EXPECT_EQ(book.ask_count(), 0);

    const Order* best = book.best_bid();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->order_id, 1);
    EXPECT_EQ(best->price_tick, 100);
    EXPECT_EQ(best->quantity, 10);
}

TEST(OrderBookTest, AddSingleAskRestsOnBook) {
    OrderBook book;

    Order ask{
        .order_id = 2,
        .seq = 2,
        .quantity = 7,
        .price_tick = 105,
        .side = OrderSide::ASK
    };

    std::vector<Trade> trades = book.add_limit(ask);

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.bid_count(), 0);
    EXPECT_EQ(book.ask_count(), 1);

    const Order* best = book.best_ask();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->order_id, 2);
    EXPECT_EQ(best->price_tick, 105);
    EXPECT_EQ(best->quantity, 7);
}

TEST(OrderBookTest, BidCrossesAskAndExecutesTrade) {
    OrderBook book;

    Order ask{
        .order_id = 10,
        .seq = 1,
        .quantity = 5,
        .price_tick = 100,
        .side = OrderSide::ASK
    };

    Order bid{
        .order_id = 11,
        .seq = 2,
        .quantity = 5,
        .price_tick = 100,
        .side = OrderSide::BID
    };

    book.add_limit(ask);
    std::vector<Trade> trades = book.add_limit(bid);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price_tick, 100);
    EXPECT_EQ(trades[0].quantity, 5);
    EXPECT_EQ(trades[0].taker_order_id, 11);
    EXPECT_EQ(trades[0].maker_order_id, 10);

    EXPECT_EQ(book.bid_count(), 0) << book.dump_book();
    EXPECT_EQ(book.ask_count(), 0) << book.dump_book();
}

TEST(OrderBookTest, PartialFillLeavesRemainingAsk) {
    OrderBook book;

    Order ask{
        .order_id = 20,
        .seq = 1,
        .quantity = 10,
        .price_tick = 100,
        .side = OrderSide::ASK
    };

    Order bid{
        .order_id = 21,
        .seq = 2,
        .quantity = 4,
        .price_tick = 100,
        .side = OrderSide::BID
    };

    book.add_limit(ask);
    std::vector<Trade> trades = book.add_limit(bid);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 4);

    EXPECT_EQ(book.bid_count(), 0) << book.dump_book();
    EXPECT_EQ(book.ask_count(), 1) << book.dump_book();

    const Order* best_ask = book.best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->order_id, 20);
    EXPECT_EQ(best_ask->price_tick, 100);
    EXPECT_EQ(best_ask->quantity, 6);
}

TEST(OrderBookTest, RemainingBidRestsAfterPartialMatch) {
    OrderBook book;

    Order ask{
        .order_id = 30,
        .seq = 1,
        .quantity = 3,
        .price_tick = 100,
        .side = OrderSide::ASK
    };

    Order bid{
        .order_id = 31,
        .seq = 2,
        .quantity = 8,
        .price_tick = 100,
        .side = OrderSide::BID
    };

    book.add_limit(ask);
    std::vector<Trade> trades = book.add_limit(bid);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 3);

    EXPECT_EQ(book.ask_count(), 0) << book.dump_book();
    EXPECT_EQ(book.bid_count(), 1) << book.dump_book();

    const Order* best_bid = book.best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->order_id, 31);
    EXPECT_EQ(best_bid->price_tick, 100);
    EXPECT_EQ(best_bid->quantity, 5);
}

TEST(OrderBookTest, BestBidIsHighestPrice) {
    OrderBook book;

    book.add_limit(Order{
        .order_id = 1,
        .seq = 1,
        .quantity = 10,
        .price_tick = 100,
        .side = OrderSide::BID
    });

    book.add_limit(Order{
        .order_id = 2,
        .seq = 2,
        .quantity = 10,
        .price_tick = 103,
        .side = OrderSide::BID
    });

    book.add_limit(Order{
        .order_id = 3,
        .seq = 3,
        .quantity = 10,
        .price_tick = 101,
        .side = OrderSide::BID
    });

    const Order* best_bid = book.best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price_tick, 103);
    EXPECT_EQ(book.bid_count(), 3);
}

TEST(OrderBookTest, BestAskIsLowestPrice) {
    OrderBook book;

    book.add_limit(Order{
        .order_id = 1,
        .seq = 1,
        .quantity = 10,
        .price_tick = 105,
        .side = OrderSide::ASK
    });

    book.add_limit(Order{
        .order_id = 2,
        .seq = 2,
        .quantity = 10,
        .price_tick = 102,
        .side = OrderSide::ASK
    });

    book.add_limit(Order{
        .order_id = 3,
        .seq = 3,
        .quantity = 10,
        .price_tick = 107,
        .side = OrderSide::ASK
    });

    const Order* best_ask = book.best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->price_tick, 102);
    EXPECT_EQ(book.ask_count(), 3);
}

TEST(OrderBookTest, CancelExistingBidRemovesIt) {
    OrderBook book;

    book.add_limit(Order{
        .order_id = 50,
        .seq = 1,
        .quantity = 9,
        .price_tick = 100,
        .side = OrderSide::BID
    });

    EXPECT_EQ(book.bid_count(), 1);

    int rc = book.cancel_order(50);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(book.bid_count(), 0);
    EXPECT_EQ(book.best_bid(), nullptr);
}

TEST(OrderBookTest, CancelMissingOrderReturnsMinusOne) {
    OrderBook book;

    book.add_limit(Order{
        .order_id = 60,
        .seq = 1,
        .quantity = 9,
        .price_tick = 100,
        .side = OrderSide::BID
    });

    int rc = book.cancel_order(999);

    EXPECT_EQ(rc, -1);
    EXPECT_EQ(book.bid_count(), 1);
}

TEST(OrderBookTest, ClearBookRemovesEverything) {
    OrderBook book;

    book.add_limit(Order{
        .order_id = 70,
        .seq = 1,
        .quantity = 5,
        .price_tick = 99,
        .side = OrderSide::BID
    });

    book.add_limit(Order{
        .order_id = 71,
        .seq = 2,
        .quantity = 6,
        .price_tick = 105,
        .side = OrderSide::ASK
    });

    EXPECT_EQ(book.bid_count(), 1);
    EXPECT_EQ(book.ask_count(), 1);

    book.clear_book();

    EXPECT_EQ(book.bid_count(), 0);
    EXPECT_EQ(book.ask_count(), 0);
    EXPECT_EQ(book.best_bid(), nullptr);
    EXPECT_EQ(book.best_ask(), nullptr);
}