#ifndef LOGGER_H
#define LOGGER_H
#include <cstdint>
#include <thread>
#include <atomic>
#include <fstream>
#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>
struct LogEvent
{
    uint64_t trade_number;
    int64_t bid, ask;
    double mid;
    int64_t trade_price;
    double inventory;
};

class Logger
{
private:
    std::queue<LogEvent> log_queue_;
    std::ofstream file_;
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;

public:
    Logger(const std::string &filename)
    {
        file_.open(filename);
        file_ << "trade_number,bid,ask,mid,trade_price,inventory\n";
    }

    void log(LogEvent e)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        log_queue_.push(std::move(e));
        cv_.notify_one();
    }

    void start()
    {
        
        worker_ = std::thread([this]
                              {
            while (true)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !log_queue_.empty() || shutdown_; });
            if (shutdown_ && log_queue_.empty()) break;
            LogEvent event = log_queue_.front();
            log_queue_.pop();
            file_ << event.trade_number << "," << event.bid << "," << event.ask << "," << event.mid << "," << event.trade_price << "," << event.inventory << "\n";

        } });
    }

    ~Logger()
    {
        shutdown_ = true;
        cv_.notify_all();
        worker_.join();
        
        
    }
};

#endif