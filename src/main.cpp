#include "engine/order_queue.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include "core/order.hpp"
#include "engine/matching_engine.hpp"
#include "strategy/market_maker.hpp"

int main(){
    OrderQueue oq;
    SPSCQueue<Trade> tq;
    double sigma = 1.0, gamma = 0.1, kappa = 1.5;
    MatchingEngine me(oq, tq);
    MarketMaker mk(tq, oq, sigma, gamma, kappa);
    me.start();
    mk.start();
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0,1);
    std::uniform_int_distribution<int64_t> bid_price_dist(90, 94);
    std::uniform_int_distribution<int64_t> ask_price_dist(96, 100);

    for(int i = 0; i<100; i++){
        Order o;
        o.id = Order::generateId();
        o.price = (o.side == Side::BID) ? bid_price_dist(rng) : ask_price_dist(rng);
        o.quantity = qty_dist(rng);
        o.side = side_dist(rng) ? Side::BID : Side::ASK;
        o.type = Type::LIMIT;
        o.timestamp_ns = std::chrono::steady_clock::now().time_since_epoch().count();
        oq.push(o);
        
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    oq.shutdown();
    std::cout << "Sent 100 orders\n";
    return 0;
}
