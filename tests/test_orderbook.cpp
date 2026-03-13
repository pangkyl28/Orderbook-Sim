#include <gtest/gtest.h>
#include <vector>
#include <cstdint>

#include "ob/OrderBook.hpp"
#include "ob/Order.hpp"
#include "ob/Trade.hpp"

using namespace ob;

TEST(OrderBookTest, EmptyBookStartsEmpty) {
    OrderBook book;

    EXPECT_EQ(book.bid_count(), 0u);
    EXPECT_EQ(book.ask_count(), 0u);
    EXPECT_EQ(book.best_bid(), nullptr);
    EXPECT_EQ(book.best_ask(), nullptr);
    EXPECT_EQ(book.bid_at(0), nullptr);
    EXPECT_EQ(book.ask_at(0), nullptr);
}

TEST(OrderBookTest, AddSingleBidRestsOnBook) {
    OrderBook book;

    Order* bid = book.create_order(OrderSide::BID, 100, 10);
    std::vector<Trade> trades = book.add_limit(*bid);

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.bid_count(), 1u);
    EXPECT_EQ(book.ask_count(), 0u);

    const Order* best = book.best_bid();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->price_tick, 100u);
    EXPECT_EQ(best->quantity, 10u);
    EXPECT_EQ(best->side, OrderSide::BID);
}


TEST(OrderBookTest, AddSingleAskRestsOnBook) {
    OrderBook book;

    std::cerr << "before create\n";
    Order* ask = book.create_order(OrderSide::ASK, 105, 7);

    std::cerr << "before add_limit\n";
    std::vector<Trade> trades = book.add_limit(*ask);

    std::cerr << "after add_limit\n";
    EXPECT_TRUE(trades.empty());

    std::cerr << "before bid_count\n";
    EXPECT_EQ(book.bid_count(), 0u);

    std::cerr << "before ask_count\n";
    EXPECT_EQ(book.ask_count(), 1u);

    std::cerr << "before best_ask\n";
    const Order* best = book.best_ask();
    ASSERT_NE(best, nullptr);

    std::cerr << "after best_ask\n";
    EXPECT_EQ(best->price_tick, 105u);
    EXPECT_EQ(best->quantity, 7u);
    EXPECT_EQ(best->side, OrderSide::ASK);
}

TEST(OrderBookTest, BidCrossesAskAndExecutesTrade) {
    OrderBook book;

    Order* ask = book.create_order(OrderSide::ASK, 100, 5);
    Order* bid = book.create_order(OrderSide::BID, 100, 5);

    std::uint64_t ask_id = ask->order_id;
    std::uint64_t bid_id = bid->order_id;

    book.add_limit(*ask);
    std::vector<Trade> trades = book.add_limit(*bid);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price_tick, 100u);
    EXPECT_EQ(trades[0].quantity, 5u);
    EXPECT_EQ(trades[0].taker_order_id, bid_id);
    EXPECT_EQ(trades[0].maker_order_id, ask_id);

    EXPECT_EQ(book.bid_count(), 0u) << book.dump_book();
    EXPECT_EQ(book.ask_count(), 0u) << book.dump_book();
    EXPECT_EQ(book.best_bid(), nullptr);
    EXPECT_EQ(book.best_ask(), nullptr);
}

TEST(OrderBookTest, PartialFillLeavesRemainingAsk) {
    OrderBook book;

    Order* ask = book.create_order(OrderSide::ASK, 100, 10);
    Order* bid = book.create_order(OrderSide::BID, 100, 4);

    std::uint64_t ask_id = ask->order_id;
    book.add_limit(*ask);
    std::vector<Trade> trades = book.add_limit(*bid);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 4u);

    EXPECT_EQ(book.bid_count(), 0u) << book.dump_book();
    EXPECT_EQ(book.ask_count(), 1u) << book.dump_book();

    const Order* best_ask = book.best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->order_id, ask_id);
    EXPECT_EQ(best_ask->price_tick, 100u);
    EXPECT_EQ(best_ask->quantity, 6u);
}

TEST(OrderBookTest, RemainingBidRestsAfterPartialMatch) {
    OrderBook book;

    Order* ask = book.create_order(OrderSide::ASK, 100, 3);
    Order* bid = book.create_order(OrderSide::BID, 100, 8);

    std::uint64_t bid_id = bid->order_id;

    book.add_limit(*ask);
    std::vector<Trade> trades = book.add_limit(*bid);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 3u);

    EXPECT_EQ(book.ask_count(), 0u) << book.dump_book();
    EXPECT_EQ(book.bid_count(), 1u) << book.dump_book();

    const Order* best_bid = book.best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->order_id, bid_id);
    EXPECT_EQ(best_bid->price_tick, 100u);
    EXPECT_EQ(best_bid->quantity, 5u);
}

TEST(OrderBookTest, BestBidIsHighestPrice) {
    OrderBook book;

    Order* bid1 = book.create_order(OrderSide::BID, 100, 10);
    Order* bid2 = book.create_order(OrderSide::BID, 103, 10);
    Order* bid3 = book.create_order(OrderSide::BID, 101, 10);

    book.add_limit(*bid1);
    book.add_limit(*bid2);
    book.add_limit(*bid3);

    const Order* best_bid = book.best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price_tick, 103u);
    EXPECT_EQ(book.bid_count(), 3u);
}

TEST(OrderBookTest, BestAskIsLowestPrice) {
    OrderBook book;

    Order* ask1 = book.create_order(OrderSide::ASK, 105, 10);
    Order* ask2 = book.create_order(OrderSide::ASK, 102, 10);
    Order* ask3 = book.create_order(OrderSide::ASK, 107, 10);

    book.add_limit(*ask1);
    book.add_limit(*ask2);
    book.add_limit(*ask3);

    const Order* best_ask = book.best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->price_tick, 102u);
    EXPECT_EQ(book.ask_count(), 3u);
}

TEST(OrderBookTest, CancelExistingBidRemovesIt) {
    OrderBook book;

    Order* bid = book.create_order(OrderSide::BID, 100, 9);
    std::uint64_t bid_id = bid->order_id;

    book.add_limit(*bid);

    EXPECT_EQ(book.bid_count(), 1u);

    int rc = book.cancel_order(bid_id);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(book.bid_count(), 0u);
    EXPECT_EQ(book.best_bid(), nullptr);
}

TEST(OrderBookTest, CancelMissingOrderReturnsMinusOne) {
    OrderBook book;

    Order* bid = book.create_order(OrderSide::BID, 100, 9);
    book.add_limit(*bid);

    int rc = book.cancel_order(999);

    EXPECT_EQ(rc, -1);
    EXPECT_EQ(book.bid_count(), 1u);
}

TEST(OrderBookTest, ClearBookRemovesEverything) {
    OrderBook book;

    Order* bid = book.create_order(OrderSide::BID, 99, 5);
    Order* ask = book.create_order(OrderSide::ASK, 105, 6);

    book.add_limit(*bid);
    book.add_limit(*ask);

    EXPECT_EQ(book.bid_count(), 1u);
    EXPECT_EQ(book.ask_count(), 1u);

    book.clear_book();

    EXPECT_EQ(book.bid_count(), 0u);
    EXPECT_EQ(book.ask_count(), 0u);
    EXPECT_EQ(book.best_bid(), nullptr);
    EXPECT_EQ(book.best_ask(), nullptr);
}

TEST(OrderBookTest, OneIncomingBidMatchesMultipleRestingAsksAcrossLevels) {
    OrderBook book;

    Order* ask1 = book.create_order(OrderSide::ASK, 100, 20);
    Order* ask2 = book.create_order(OrderSide::ASK, 100, 30);
    Order* ask3 = book.create_order(OrderSide::ASK, 101, 10);
    Order* bid  = book.create_order(OrderSide::BID, 101, 55);

    std::uint64_t ask1_id = ask1->order_id;
    std::uint64_t ask2_id = ask2->order_id;
    std::uint64_t ask3_id = ask3->order_id;
    std::uint64_t bid_id  = bid->order_id;

    book.add_limit(*ask1);
    book.add_limit(*ask2);
    book.add_limit(*ask3);

    std::vector<Trade> trades = book.add_limit(*bid);

    ASSERT_EQ(trades.size(), 3u);

    EXPECT_EQ(trades[0].price_tick, 100u);
    EXPECT_EQ(trades[0].quantity, 20u);
    EXPECT_EQ(trades[0].taker_order_id, bid_id);
    EXPECT_EQ(trades[0].maker_order_id, ask1_id);

    EXPECT_EQ(trades[1].price_tick, 100u);
    EXPECT_EQ(trades[1].quantity, 30u);
    EXPECT_EQ(trades[1].taker_order_id, bid_id);
    EXPECT_EQ(trades[1].maker_order_id, ask2_id);

    EXPECT_EQ(trades[2].price_tick, 101u);
    EXPECT_EQ(trades[2].quantity, 5u);
    EXPECT_EQ(trades[2].taker_order_id, bid_id);
    EXPECT_EQ(trades[2].maker_order_id, ask3_id);

    EXPECT_EQ(book.bid_count(), 0u) << book.dump_book();
    EXPECT_EQ(book.ask_count(), 1u) << book.dump_book();

    const Order* best_ask = book.best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->price_tick, 101u);
    EXPECT_EQ(best_ask->quantity, 5u);
    EXPECT_EQ(best_ask->order_id, ask3_id);
}

TEST(OrderBookTest, FIFOAtSamePriceOlderAskMatchesFirst) {
    OrderBook book;

    Order* ask1 = book.create_order(OrderSide::ASK, 100, 4);
    Order* ask2 = book.create_order(OrderSide::ASK, 100, 6);
    Order* bid  = book.create_order(OrderSide::BID, 100, 5);

    std::uint64_t ask1_id = ask1->order_id;
    std::uint64_t ask2_id = ask2->order_id;
    std::uint64_t bid_id  = bid->order_id;

    book.add_limit(*ask1);
    book.add_limit(*ask2);

    std::vector<Trade> trades = book.add_limit(*bid);

    ASSERT_EQ(trades.size(), 2u);

    EXPECT_EQ(trades[0].maker_order_id, ask1_id);
    EXPECT_EQ(trades[0].taker_order_id, bid_id);
    EXPECT_EQ(trades[0].quantity, 4u);

    EXPECT_EQ(trades[1].maker_order_id, ask2_id);
    EXPECT_EQ(trades[1].taker_order_id, bid_id);
    EXPECT_EQ(trades[1].quantity, 1u);

    EXPECT_EQ(book.ask_count(), 1u);
    const Order* best_ask = book.best_ask();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->order_id, ask2_id);
    EXPECT_EQ(best_ask->quantity, 5u);
}

TEST(OrderBookTest, PricePriorityHigherBidMatchesFirst) {
    OrderBook book;

    Order* bid1 = book.create_order(OrderSide::BID, 99, 10);
    Order* bid2 = book.create_order(OrderSide::BID, 101, 10);
    Order* ask  = book.create_order(OrderSide::ASK, 100, 6);

    std::uint64_t bid2_id = bid2->order_id;
    std::uint64_t ask_id  = ask->order_id;

    book.add_limit(*bid1);
    book.add_limit(*bid2);

    std::vector<Trade> trades = book.add_limit(*ask);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].maker_order_id, bid2_id);
    EXPECT_EQ(trades[0].taker_order_id, ask_id);
    EXPECT_EQ(trades[0].price_tick, 101u);
    EXPECT_EQ(trades[0].quantity, 6u);

    const Order* best_bid = book.best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->price_tick, 101u);
    EXPECT_EQ(best_bid->quantity, 4u);
}

TEST(OrderBookTest, CancelOneOrderAtLevelLeavesOtherOrderAtSameLevel) {
    OrderBook book;

    Order* bid1 = book.create_order(OrderSide::BID, 100, 5);
    Order* bid2 = book.create_order(OrderSide::BID, 100, 7);

    std::uint64_t bid1_id = bid1->order_id;
    std::uint64_t bid2_id = bid2->order_id;

    book.add_limit(*bid1);
    book.add_limit(*bid2);

    EXPECT_EQ(book.bid_count(), 2u);

    int rc = book.cancel_order(bid1_id);
    EXPECT_EQ(rc, 0);

    EXPECT_EQ(book.bid_count(), 1u);

    const Order* best_bid = book.best_bid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->order_id, bid2_id);
    EXPECT_EQ(best_bid->price_tick, 100u);
    EXPECT_EQ(best_bid->quantity, 7u);
}

TEST(OrderBookTest, CancelLastOrderAtLevelRemovesEntirePriceLevel) {
    OrderBook book;

    Order* bid1 = book.create_order(OrderSide::BID, 101, 5);
    Order* bid2 = book.create_order(OrderSide::BID, 100, 7);

    std::uint64_t bid1_id = bid1->order_id;

    book.add_limit(*bid1);
    book.add_limit(*bid2);

    ASSERT_NE(book.best_bid(), nullptr);
    EXPECT_EQ(book.best_bid()->price_tick, 101u);

    int rc = book.cancel_order(bid1_id);
    EXPECT_EQ(rc, 0);

    ASSERT_NE(book.best_bid(), nullptr);
    EXPECT_EQ(book.best_bid()->price_tick, 100u);
    EXPECT_EQ(book.bid_count(), 1u);
}

TEST(OrderBookTest, BidAtTraversesBidsInPriceTimePriorityOrder) {
    OrderBook book;

    Order* bid1 = book.create_order(OrderSide::BID, 101, 5);
    Order* bid2 = book.create_order(OrderSide::BID, 101, 6);
    Order* bid3 = book.create_order(OrderSide::BID, 100, 7);

    std::uint64_t bid1_id = bid1->order_id;
    std::uint64_t bid2_id = bid2->order_id;
    std::uint64_t bid3_id = bid3->order_id;

    book.add_limit(*bid1);
    book.add_limit(*bid2);
    book.add_limit(*bid3);

    const Order* o0 = book.bid_at(0);
    const Order* o1 = book.bid_at(1);
    const Order* o2 = book.bid_at(2);
    const Order* o3 = book.bid_at(3);

    ASSERT_NE(o0, nullptr);
    ASSERT_NE(o1, nullptr);
    ASSERT_NE(o2, nullptr);
    EXPECT_EQ(o3, nullptr);

    EXPECT_EQ(o0->order_id, bid1_id);
    EXPECT_EQ(o1->order_id, bid2_id);
    EXPECT_EQ(o2->order_id, bid3_id);
}

TEST(OrderBookTest, AskAtTraversesAsksInPriceTimePriorityOrder) {
    OrderBook book;

    Order* ask1 = book.create_order(OrderSide::ASK, 100, 5);
    Order* ask2 = book.create_order(OrderSide::ASK, 100, 6);
    Order* ask3 = book.create_order(OrderSide::ASK, 102, 7);

    std::uint64_t ask1_id = ask1->order_id;
    std::uint64_t ask2_id = ask2->order_id;
    std::uint64_t ask3_id = ask3->order_id;

    book.add_limit(*ask1);
    book.add_limit(*ask2);
    book.add_limit(*ask3);

    const Order* o0 = book.ask_at(0);
    const Order* o1 = book.ask_at(1);
    const Order* o2 = book.ask_at(2);
    const Order* o3 = book.ask_at(3);

    ASSERT_NE(o0, nullptr);
    ASSERT_NE(o1, nullptr);
    ASSERT_NE(o2, nullptr);
    EXPECT_EQ(o3, nullptr);

    EXPECT_EQ(o0->order_id, ask1_id);
    EXPECT_EQ(o1->order_id, ask2_id);
    EXPECT_EQ(o2->order_id, ask3_id);
}