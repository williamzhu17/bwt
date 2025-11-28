#include "blocking_queue.hpp"

template<typename T>
void BlockingQueue<T>::push(const T& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(item);
    condition_.notify_one();
}

template<typename T>
bool BlockingQueue<T>::pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until queue has items or is closed
    condition_.wait(lock, [this] { return !queue_.empty() || closed_; });
    
    // If queue is empty and closed, return false
    if (queue_.empty() && closed_) {
        return false;
    }
    
    // Pop item
    item = queue_.front();
    queue_.pop();
    return true;
}

template<typename T>
void BlockingQueue<T>::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    condition_.notify_all();
}

template<typename T>
bool BlockingQueue<T>::is_closed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}
