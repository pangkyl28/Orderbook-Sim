Order Types suppoerted:
- Limit
- Market
- Cancel by order_id

Matching Rule: Price-time priority (FIFO at each price)
Fill behavior: Partial fills allowed
Price Type: Integer ticks
Time: Monotonic sequence number per order
Outputs:
- Trades (trade_id, price, qty, taker_order_id, maker_order_id)
- Top of book (best bid/ask and sizes)