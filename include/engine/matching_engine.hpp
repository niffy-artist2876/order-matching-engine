#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H
#include <thread>
#include <atomic>
#include "core/order_book.hpp"
#include "engine/order_queue.hpp"
#include "threading/spsc_queue.hpp"
#include "core/trade.hpp"

class MatchingEngine{
    private:
        OrderBook book_;
        OrderQueue& order_queue_;
        SPSCQueue<Trade>& trade_queue_;
        std::thread worker_;
        std::atomic<bool> running_{false};
    
    public:
        MatchingEngine(OrderQueue& oq, SPSCQueue<Trade>& tq): order_queue_(oq), trade_queue_(tq) {}

        ~MatchingEngine(){
            order_queue_.shutdown();
            running_ = false;
            worker_.join();
        }

        void start(){
            running_=true;
            worker_ = std::thread([this]{
                while(true){
                    std::optional<Order> popped = order_queue_.pop();
                    if(popped==std::nullopt) break;
                    Order o = popped.value();
                    if(o.type == Type::CANCEL){
                        book_.cancelOrder(o.id);
                    }
                    else{
                        book_.addOrder(o);
                        auto trades = book_.match();
                        for(auto& t: trades){
                            trade_queue_.push(t);
                            auto [best_bid, best_ask] = book_.getBBO();
                        }
                    }
                }
                
            });
        }
};
#endif