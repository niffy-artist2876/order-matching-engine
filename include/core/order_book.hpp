#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H
#include <map>
#include <deque>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <algorithm>
#include <chrono>
#include "core/order.hpp"
#include "core/trade.hpp"
#include <unordered_map>

typedef std::deque<Order> PriceLevel;

class OrderBook{
    private:
        std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;
        std::map<int64_t, PriceLevel> asks_;
        std::unordered_map<uint64_t, std::pair<Side, int64_t>> order_side_index_;
        mutable std::shared_mutex rw_mutex_;

    public:
        void addOrder(Order o){
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            o.side == Side::BID ? bids_[o.price].push_back(o) : asks_[o.price].push_back(o); 
            order_side_index_[o.id] = {o.side, o.price};
        }

        void cancelOrder(uint64_t id){
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            if(order_side_index_.find(id) == order_side_index_.end()) return;
            auto [side, price] = order_side_index_[id];
            if(side == Side::BID){
                auto it = std::find_if(bids_[price].begin(), bids_[price].end(), [id](const Order& o){return o.id==id;});
                if(it != bids_[price].end()) bids_[price].erase(it);
                if(bids_[price].empty()) bids_.erase(price);
            }
            else{
                auto it = std::find_if(asks_[price].begin(), asks_[price].end(), [id](const Order& o){return o.id==id;});
                if(it != asks_[price].end()) asks_[price].erase(it);
                if(asks_[price].empty()) asks_.erase(price);
            }
            order_side_index_.erase(id);

        }

        std::vector<Trade> match(){
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            std::vector<Trade> trades; 
            while(!bids_.empty() && !asks_.empty() && bids_.begin()->first >= asks_.begin()->first){
                Order bid_front = bids_.begin()->second.front(), ask_front = asks_.begin()->second.front();
                uint64_t trade_qty = std::min(bid_front.quantity, ask_front.quantity);
                uint64_t buy_order_id = bid_front.id;
                uint64_t sell_order_id = ask_front.id;
                int64_t price = ask_front.price;     
                uint64_t quantity = trade_qty;
                int64_t timestamp_ns = std::chrono::steady_clock::now().time_since_epoch().count();
                Trade t = {buy_order_id, sell_order_id, price, quantity, timestamp_ns, Side::ASK};
                bids_.begin()->second.front().quantity -= trade_qty;
                asks_.begin()->second.front().quantity -= trade_qty;
                if(bids_.begin()->second.front().quantity == 0){
                    order_side_index_.erase(bid_front.id);
                    bids_.begin()->second.pop_front();
                    if(bids_.begin()->second.empty()) bids_.erase(bids_.begin());
                }
                if(asks_.begin()->second.front().quantity == 0){
                    order_side_index_.erase(ask_front.id);
                    asks_.begin()->second.pop_front();
                    if(asks_.begin()->second.empty()) asks_.erase(asks_.begin());
                }
                trades.push_back(t);
                
            }     
            return trades;      
        }

        std::pair<int64_t, int64_t> getBBO(){
            std::shared_lock<std::shared_mutex> lock(rw_mutex_);
            if(bids_.empty() || asks_.empty()) return {};
            return {bids_.begin()->first, asks_.begin()->first};

        }
};

#endif