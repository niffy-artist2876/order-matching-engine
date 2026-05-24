#ifndef TRADE_H
#define TRADE_H

#include <cstdint>
#include "core/order.hpp"

struct Trade{
	uint64_t buy_order_id, sell_order_id;
	int64_t price;
	uint64_t quantity;
	int64_t timestamp_ns;
	Side aggressor_side;
	
};
#endif
