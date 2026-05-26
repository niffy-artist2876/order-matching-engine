#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H
#include <thread>
#include <atomic>
#include "core/order_book.hpp"
#include "engine/order_queue.hpp"
#include "threading/spsc_queue.hpp"
#include "core/trade.hpp"
#include "logging/logger.hpp"

class MatchingEngine
{
private:
    OrderBook book_;
    OrderQueue &order_queue_;
    SPSCQueue<Trade> &trade_queue_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    Logger &logger_;

public:
    MatchingEngine(OrderQueue &oq, SPSCQueue<Trade> &tq, Logger &log) : order_queue_(oq), trade_queue_(tq), logger_(log) {}

    ~MatchingEngine()
    {
        order_queue_.shutdown();
        running_ = false;
        worker_.join();
    }

    OrderBook& getBook() { return book_; }

    void start()
    {
        running_ = true;
        worker_ = std::thread([this]
                              {
                                  while (true)
                                  {
                                      std::optional<Order> popped = order_queue_.pop();
                                      if (popped == std::nullopt){
                                        if(!running_) break;
                                        continue;
                                      }
                                      Order o = popped.value();
                                      if (o.type == Type::CANCEL)
                                      {
                                          book_.cancelOrder(o.id);
                                      }
                                      else
                                      {
                                          book_.addOrder(o);
                                          auto trades = book_.match();
                                          for (auto &t : trades)
                                          {
                                              trade_queue_.push(t);
                                          }
                                      }
                                  }
                              });
    }
};
#endif