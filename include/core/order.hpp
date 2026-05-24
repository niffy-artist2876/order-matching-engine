#ifndef ORDER_H
#define ORDER_H
#include <atomic>
#include <cstdint>

enum class Side { BID, ASK };
enum class Type { LIMIT, MARKET, CANCEL };

struct Order{
	uint64_t id;
	int64_t price;
	uint64_t quantity;
	int64_t timestamp_ns;
	Side side;
	Type type;
	inline static std::atomic<uint64_t> next_id_;
	static uint64_t generateId() { return Order::next_id_.fetch_add(1, std::memory_order_relaxed); }
};
#endif
