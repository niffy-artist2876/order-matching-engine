#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H
#include <atomic>

template<typename T>
class SPSCQueue{
    private:
        static constexpr size_t CAPACITY = 1024;
        T buffer_[CAPACITY];
        alignas(64) std::atomic<size_t> head_{0};
        alignas(64) std::atomic<size_t> tail_{0};

    public:
        bool push(T item){
            size_t h = head_.load(std::memory_order_relaxed);
            size_t next = (h+1)&(CAPACITY-1);
            size_t t = tail_.load(std::memory_order_acquire);
            if(next==t){
                return false;
            }
            buffer_[h] = std::move(item);
            head_.store(std::memory_order_release);
            return true;
        }

        bool pop(T& item){
            size_t t = tail_.load(std::memory_order_relaxed);
            size_t h = head_.load(std::memory_order_acquire);
            if(t==h) return false;
            item = std::move(buffer_[t]);
            tail_.store((t+1)&(CAPACITY-1), std::memory_order_release);
            return true;
        }
};

#endif