#ifndef MARKET_MAKER_H
#define MARKET_MAKER_H
#include "threading/spsc_queue.hpp"
#include "engine/order_queue.hpp"
#include <thread>
#include <atomic>
#include <cmath>
#include <chrono>
#include <deque>
#include <memory>
#include <numeric>
#include <algorithm>
#include <unordered_set>
#include "core/trade.hpp"
#include "logging/logger.hpp"
#include "core/order_book.hpp"

class MarketMaker
{
private:
    SPSCQueue<Trade> &trade_queue_;
    OrderQueue &order_queue_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    OrderBook &order_book_;
    double sigma_, gamma_, kappa_;
    double mid_price_, inventory_;
    int64_t last_bid_ = 0, last_ask_ = 0;
    uint64_t bid_id_, ask_id_;
    Logger &logger_;
    std::unordered_set<uint64_t> active_bid_ids_;
    std::unordered_set<uint64_t> active_ask_ids_;
    double max_inventory_;
    uint64_t quote_size_;
    std::deque<double> price_history_;
    size_t vol_window_;
    std::shared_ptr<std::atomic<uint64_t>> messages_processed_;
    uint64_t total_messages_;

public:
    MarketMaker(SPSCQueue<Trade> &tq, OrderQueue &oq, OrderBook &ob, Logger &log,
                double sigma, double gamma, double kappa,
                std::shared_ptr<std::atomic<uint64_t>> messages_processed,
                uint64_t total_messages,
                double max_inventory = 50.0, uint64_t quote_size = 10,
                size_t vol_window = 500)
        : trade_queue_(tq), order_queue_(oq), order_book_(ob), logger_(log),
          sigma_(sigma), kappa_(kappa), gamma_(gamma), mid_price_(95.0), inventory_(0.0),
          bid_id_(0), ask_id_(0), max_inventory_(max_inventory), quote_size_(quote_size),
          vol_window_(vol_window), messages_processed_(messages_processed),
          total_messages_(total_messages) {}

    ~MarketMaker()
    {
        running_ = false;
        worker_.join();
    }

    void start()
    {
        running_ = true;
        worker_ = std::thread([this]
                              {
            while (true)
            {
                if (!running_) break;
                Trade trade;
                int trade_count = 0;
                while (trade_queue_.pop(trade))
                {
                    if (trade.price > 0){
                        double price_dollars = trade.price / 100.0; 
                        price_history_.push_back(price_dollars);
                        if (price_history_.size() > vol_window_)
                            price_history_.pop_front();
                        mid_price_ = trade.price; 
                    }
                    if (active_bid_ids_.count(trade.buy_order_id))
                        inventory_ += trade.quantity;
                    if (active_ask_ids_.count(trade.sell_order_id))
                        inventory_ -= trade.quantity;
                    if (last_bid_ > 0 && last_ask_ > 0)
                    {
                        LogEvent event = {trade.buy_order_id, last_bid_, last_ask_, mid_price_, trade.price, inventory_};
                        logger_.log(event);
                    }
                    trade_count++;
                }
                Order c1, c2;
                c1.type = Type::CANCEL;
                c2.type = Type::CANCEL;
                c1.id = bid_id_;
                c2.id = ask_id_;
                order_queue_.push(c1);
                order_queue_.push(c2);
                auto [best_bid, best_ask] = order_book_.getBBO();
                if (best_bid > 0 && best_ask > 0) {
                    mid_price_ = (best_bid + best_ask) / 2.0;
                    double mid_dollars = mid_price_ / 100.0;
                    price_history_.push_back(mid_dollars);
                    if (price_history_.size() > vol_window_)
                        price_history_.pop_front();
                }
                auto [bid, ask] = computeQuotes();
                if (bid == 0 && ask == 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                Order l1, l2;
                l1.side = Side::BID;
                l2.side = Side::ASK;
                l1.type = Type::LIMIT;
                l2.type = Type::LIMIT;
                l1.price = static_cast<int64_t>(std::round(bid));
                l2.price = static_cast<int64_t>(std::round(ask));
                last_bid_ = l1.price;
                last_ask_ = l2.price;
                active_bid_ids_.erase(bid_id_);
                active_ask_ids_.erase(ask_id_);
                l1.id = Order::generateId();
                l2.id = Order::generateId();
                active_bid_ids_.insert(l1.id);
                active_ask_ids_.insert(l2.id);
                l1.quantity = quote_size_;
                l2.quantity = quote_size_;
                if (inventory_ > 20) {
                    order_queue_.push(l2); 
                } else if (inventory_ < -20) {
                    order_queue_.push(l1); 
                } else {
                    order_queue_.push(l1);
                    order_queue_.push(l2);
                }   
                bid_id_ = l1.id;
                ask_id_ = l2.id;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } });
    }

    double computeRollingSigma()
    {
        if (price_history_.size() < 2)
            return sigma_;
        std::vector<double> returns;
        returns.reserve(price_history_.size() - 1);
        for (size_t i = 1; i < price_history_.size(); i++)
        {
            if (price_history_[i - 1] > 0 && price_history_[i] != price_history_[i - 1])
                returns.push_back(std::log(price_history_[i] / price_history_[i - 1]));
        }
        if (returns.empty())
            return sigma_;
        double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
        double var = 0.0;
        for (double r : returns)
            var += (r - mean) * (r - mean);
        return std::sqrt(var / returns.size());
    }

    std::pair<double, double> computeQuotes()
    {
        if (mid_price_ < 1000)
            return {0, 0};

        double sigma = computeRollingSigma();
        double T = std::max(0.01, 1.0 - static_cast<double>(messages_processed_->load(std::memory_order_relaxed)) /
                                         static_cast<double>(total_messages_));
        double mid_dollars = mid_price_ / 100.0;
        double r = mid_dollars - inventory_ * gamma_ * sigma * sigma * T;
        double delta = gamma_ * sigma * sigma * T + (2.0 / gamma_) * std::log(1.0 + gamma_ / kappa_);

        double bid = r - delta / 2.0;
        double ask = r + delta / 2.0;
        return {bid * 100.0, ask * 100.0};
    }
};
#endif
