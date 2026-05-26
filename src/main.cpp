#include "engine/order_queue.hpp"
#include <iostream>
#include <chrono>
#include "core/order.hpp"
#include "engine/matching_engine.hpp"
#include "strategy/market_maker.hpp"
#include "logging/logger.hpp"
#include "data/lobster_parser.hpp"
#include "core/order_book.hpp"

int main(){
      OrderQueue oq;
      SPSCQueue<Trade> tq;
      Logger logger("trades.csv");
      double sigma = 0.01, gamma = 0.05, kappa = 100.0;
      double max_inventory = 50.0;
      uint64_t quote_size = 10;
      size_t vol_window = 100;
      LobsterParser parser("message_file.csv", oq);
      MatchingEngine me(oq, tq, logger);
      MarketMaker mk(tq, oq, me.getBook(), logger, sigma, gamma, kappa,
                     parser.progressCounter(), parser.totalMessages(),
                     max_inventory, quote_size, vol_window);
      logger.start();
      me.start();
      mk.start();
      parser.replay();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      oq.shutdown();
      std::cout << "Done replaying LOBSTER data\n";
      return 0;
  }
