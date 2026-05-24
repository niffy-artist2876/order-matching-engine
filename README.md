# Order Matching Engine 
A C++ engine which takes orders for a given stock, matches the corresponding bids and asks and facilitates trade for that stock.

**Note:** Work in Progress, so don't run it right now pls :(

## Key Features:
- Multithreaded C++ engine.
- Uses a dedicated market maker thread to quote bids and asks using the Avellaneda-Stoikov Model.
- Bid and Ask orders are stored in a reader-writer locked order book which sorts these orders by price-time priority (best bid/ask price at the earliest time stored as a deque within an ordered map).
- Uses a lock-free SPSC queue to publish trades from the matching engine thread to the market maker thread, allowing the latter thread to update its bid and ask quotes in real-time.
- Uses synchronization primitives to avoid race conditions.

## How to run:
1. Clone this repository.
2. Make sure you have cmake installed using ``cmake --version``.
3. If not, install it using ``sudo apt install cmake``.
4. Navigate to the cloned repository and run ``mkdir build && cd build``
5. Run ``cmake ..``.
6. Run ``make``.
7. Run ``./hft_lob``.

## References
- Avellaneda, M. & Stoikov, S. (2008). *High-frequency trading in a limit order book*. Quantitative Finance, 8(3), 217-224.
