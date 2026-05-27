#ifndef LOBSTER_PARSER
#define LOBSTER_PARSER
#include <string>
#include <fstream>
#include <sstream>
#include <atomic>
#include <memory>
#include <unordered_map>
#include "core/order.hpp"
#include "engine/order_queue.hpp"
#include <thread>
#include <chrono>

class LobsterParser
{
private:
    std::ifstream message_file_;
    OrderQueue &order_queue_;
    uint64_t total_messages_;
    std::shared_ptr<std::atomic<uint64_t>> messages_processed_;
    std::unordered_map<uint64_t, uint64_t> id_map_; // lobster_id -> generated_id

    static uint64_t countExecutions(const std::string &path) {
        std::ifstream f(path);
        std::string line, token;
        uint64_t count = 0;
        while (std::getline(f, line)) {
            std::istringstream ss(line);
            std::getline(ss, token, ','); // time
            std::getline(ss, token, ','); // type
            int type = std::stoi(token);
            if (type == 4 || type == 5) count++;
        }
        return count;
    }

public:
    LobsterParser(const std::string &message_path, OrderQueue &oq)
        : message_file_(message_path), order_queue_(oq),
          total_messages_(countExecutions(message_path)),
          messages_processed_(std::make_shared<std::atomic<uint64_t>>(0)) {}

    uint64_t totalMessages() const { return total_messages_; }
    std::shared_ptr<std::atomic<uint64_t>> progressCounter() { return messages_processed_; }

    void replay()
    {
        if (!message_file_.is_open())
            return;
        std::string line;
        int count = 0;
        while (std::getline(message_file_, line))
        {
            std::stringstream ss(line);
            std::string token;
            
            double time;
            int type;
            uint64_t order_id;
            uint64_t size;
            int64_t price;
            int direction;

            std::getline(ss, token, ',');
            time = std::stod(token);
            std::getline(ss, token, ',');
            type = std::stoi(token);
            std::getline(ss, token, ',');
            order_id = std::stoull(token);
            std::getline(ss, token, ',');
            size = std::stoull(token);
            std::getline(ss, token, ',');
            price = std::stoll(token);
            std::getline(ss, token, ',');
            direction = std::stoi(token);
            if (type == 1)
            {
                uint64_t gen_id = Order::generateId();
                id_map_[order_id] = gen_id;
                Order o;
                o.id = gen_id;
                o.price = price;
                o.quantity = size;
                o.side = (direction == 1) ? Side::BID : Side::ASK;
                o.type = Type::LIMIT;
                o.timestamp_ns = static_cast<int64_t>(time * 1e9);
                order_queue_.push(o);
                count++;
                if (count % 100 == 0)
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
            else if (type == 2 || type == 3)
            {
                auto it = id_map_.find(order_id);
                if (it == id_map_.end()) continue;
                Order o;
                o.id = it->second;
                o.type = Type::CANCEL;
                id_map_.erase(it);
                order_queue_.push(o);
            }
            else if (type == 4 || type == 5)
            {
                messages_processed_->fetch_add(1, std::memory_order_relaxed);
                uint64_t gen_id = Order::generateId();
                id_map_[order_id] = gen_id;
                Order o;
                o.side = (direction == 1) ? Side::ASK : Side::BID;
                o.type = Type::LIMIT;
                o.price = price;
                o.quantity = size;
                o.id = gen_id;
                order_queue_.push(o);
            }
        }
    }
};
#endif