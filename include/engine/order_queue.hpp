#ifndef ORDER_QUEUE_H
#define ORDER_QUEUE_H
#include <queue>
#include <mutex>
#include <optional>
#include <condition_variable>
#include "core/order.hpp"

class OrderQueue{
private:
	std::queue<Order> queue_;
	std::mutex mutex_;
	std::condition_variable cv_;
	bool shutdown_ = false;

public:
	void push(Order o){
		std::unique_lock<std::mutex> lock(mutex_);
		queue_.push(std::move(o));
		cv_.notify_one();
	}
	

	std::optional<Order> pop(){
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [this]{return !queue_.empty() || shutdown_; });
		if(shutdown_ && queue_.empty()) return std::nullopt;
		Order f = std::move(queue_.front());
		queue_.pop();
		return f;
	}

	void shutdown(){
		std::unique_lock<std::mutex> lock(mutex_);
		shutdown_ = true;
		cv_.notify_all();
	}

	
};
#endif
