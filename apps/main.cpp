#include <iostream>
#include "ob/OrderBook.hpp"
#include <vector>
#include <cassert>
using namespace std;



void print_trade(const Trade& trade)
{
    cout << "Trade ID: " << trade.trade_id << ", Price Tick: " << trade.price_tick 
         << ", Quantity: " << trade.quantity << ", Taker Order ID: " << trade.taker_order_id 
         << ", Maker Order ID: " << trade.maker_order_id << endl;
    return;
}

void print_order(const Order& order)
{
    cout << "Order ID: " << order.order_id << ", Side: " << (order.side == OrderSide::BID ? "BID" : "ASK") 
         << ", Price Tick: " << order.price_tick << ", Quantity: " << order.quantity 
         << ", Seq: " << order.seq << endl;
}

void print_orderbook(const ob::OrderBook& ob)
{
    vector<Order> bids = ob.get_bids();
    vector<Order> asks = ob.get_asks();
    
    int n = bids.size();
    cout << "Bids:" << endl;
    for (int i = 0; i < n; i++)
    {
        print_order(bids[i]);
    }
    cout << "Asks:" << endl;
    n = asks.size();
    for (int i = 0; i < n; i++)
    {
        print_order(asks[i]);
    }
    return;
}

int main() {
    cout << "Hello, Orderbook Simulator!\n" << endl;

    ob::OrderBook book;

    // test 1: add a limit bid and ask that should match and empty the book
    cout << "Test 1: Add a limit bid and ask that should match and empty the book" << endl;
    Order* o1 = book.create_order(OrderSide::BID, 100, 10);
    Order* o2 = book.create_order(OrderSide::ASK, 100, 10);
    vector<Trade> trades = book.add_limit(*o1);
    assert(trades.size() == 0);
    trades = book.add_limit(*o2);
    assert(trades.size() == 1);
    print_trade(trades[0]);
    print_orderbook(book);
    cout << "-----------------------------" << endl;

    // test 2: add a limit bid that partially matches with an existing ask
    cout << "Test 2: Add a limit bid that partially matches with an existing ask" << endl;
    Order* o3 = book.create_order(OrderSide::ASK, 100, 10);
    Order* o4 = book.create_order(OrderSide::BID, 100, 5);
    trades = book.add_limit(*o3);
    trades = book.add_limit(*o4);
    assert(trades.size() == 1);
    print_trade(trades[0]);
    print_orderbook(book);
    cout << "-----------------------------" << endl;

    // test 3: add a limit bid at a different price that should match with an existing ask
    cout << "Test 3: Add a limit bid at a different price that should match with an existing ask" << endl;
    Order* o5 = book.create_order(OrderSide::BID, 101, 2);
    trades = book.add_limit(*o5);
    assert(trades.size() == 1);
    print_trade(trades[0]);
    print_orderbook(book);
    cout << "-----------------------------" << endl;

    // test 4: add a limit bid at a different price that should not match with an existing ask
    cout << "Test 4: Add a limit bid at a different price that should not match with an existing ask" << endl;
    Order* o6 = book.create_order(OrderSide::BID, 99, 2);
    trades = book.add_limit(*o6);
    assert(trades.size() == 0);
    print_orderbook(book);
    cout << "-----------------------------" << endl;

    return 0;
}
