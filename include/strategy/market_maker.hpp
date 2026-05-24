#ifndef MARKET_MAKER_H
#define MARKET_MAKER_H
#include "threading/spsc_queue.hpp"
#include "engine/order_queue.hpp"
#include <thread>
#include <atomic>
#include <cmath>
#include <chrono>
#include "core/trade.hpp"
class MarketMaker{
    private:
        SPSCQueue<Trade>& trade_queue_;
        OrderQueue& order_queue_;
        std::thread worker_;
        std::atomic<bool> running_{false};
        double sigma_, gamma_, kappa_;
        double mid_price_, inventory_;
        uint64_t bid_id_, ask_id_;

    public:
        MarketMaker(SPSCQueue<Trade>& tq, OrderQueue& oq, double sigma, double gamma, double kappa) : trade_queue_(tq), order_queue_(oq),
        sigma_(sigma), kappa_(kappa), gamma_(gamma), mid_price_(95.0), inventory_(0.0), bid_id_(0), ask_id_(0){}

        ~MarketMaker(){
            running_ = false;
            worker_.join();
        }

        void start(){
            running_ = true;
            worker_ = std::thread([this]{
                while(true){
                    if(!running_) break;
                    Trade trade;
                    bool popped = trade_queue_.pop(trade);
                    if(popped){
                        if (trade.price > 0) mid_price_ = trade.price;
                        if (trade.aggressor_side == Side::ASK) inventory_ -= trade.quantity;
                        else inventory_ += trade.quantity;
                    } 
                    Order c1, c2;
                    c1.type = Type::CANCEL;
                    c2.type = Type::CANCEL;
                    c1.id = bid_id_;
                    c2.id = ask_id_;
                    order_queue_.push(c1);
                    order_queue_.push(c2);
                    auto [bid, ask] = computeQuotes();
                    Order l1, l2;
                    l1.side = Side::BID;
                    l2.side = Side::ASK;
                    l1.type = Type::LIMIT;
                    l2.type = Type::LIMIT;
                    l1.price = static_cast<int64_t>(bid);
                    l2.price = static_cast<int64_t>(ask);
                    l1.id = Order::generateId();
                    l2.id = Order::generateId();
                    l1.quantity = 10;
                    l2.quantity = 10;
                    if (inventory_ > -50) order_queue_.push(l2);  
                    if (inventory_ < 50)  order_queue_.push(l1); 
                    bid_id_ = l1.id;
                    ask_id_ = l2.id;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
            });
        }

        std::pair<double, double> computeQuotes(){
            double T = 1.0;
            double r = mid_price_ - inventory_*gamma_*sigma_*sigma_*T;
            double delta = gamma_*sigma_*sigma_*T + (2.0/gamma_)*log(1.0 + gamma_/kappa_);
            double bid = r-delta/2.0;
            double ask = r+delta/2.0;
            return {bid, ask};
        }
};
#endif