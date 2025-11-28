#ifndef BLOCKING_QUEUE_HPP
#define BLOCKING_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * Simple thread-safe blocking queue.
 * Supports push, pop (blocking), and close operations.
 */
template<typename T>
class BlockingQueue {
public:
    /**
     * Push an item onto the queue.
     * @param item Item to push
     */
    void push(const T& item);
    
    /**
     * Pop an item from the queue.
     * Blocks until an item is available or the queue is closed.
     * @param item Reference to store the popped item
     * @return true if an item was popped, false if queue is closed
     */
    bool pop(T& item);
    
    /**
     * Close the queue.
     * Wakes up all waiting threads. After closing, push operations
     * are still allowed, but pop will return false once queue is empty.
     */
    void close();
    
    /**
     * Check if the queue is closed.
     * @return true if closed, false otherwise
     */
    bool is_closed() const;

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool closed_ = false;
};

#include "blocking_queue.cpp"

#endif // BLOCKING_QUEUE_HPP
