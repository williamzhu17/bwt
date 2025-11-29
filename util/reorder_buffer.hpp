#ifndef REORDER_BUFFER_HPP
#define REORDER_BUFFER_HPP

#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <limits>

/**
 * Thread-safe reorder buffer that allows workers to produce results
 * out of order, while a writer consumes them in the correct sequence.
 * 
 * Workers place results into specific slots (by index), and the writer
 * always consumes the next required block in order (0, 1, 2, ...).
 * 
 * The buffer has a fixed capacity to prevent unbounded memory growth.
 * If the buffer is full, workers wait until space is available.
 * The writer waits until the next expected block is ready.
 */
template<typename T>
class ReorderBuffer {
public:
    /**
     * Construct a reorder buffer with the specified capacity.
     * @param capacity Maximum number of blocks that can be in flight
     */
    explicit ReorderBuffer(size_t capacity);
    
    /**
     * Place a result at a specific index (called by worker threads).
     * Blocks if the buffer is full (all slots in the current window are occupied).
     * @param index The index of this result (must be >= next_expected_index_)
     * @param item The result to store
     */
    void put(size_t index, const T& item);
    
    /**
     * Get the next expected result in order (called by writer thread).
     * Blocks until the next block (next_expected_index_) is ready.
     * @param item Reference to store the result
     * @return true if an item was retrieved, false if buffer is closed
     */
    bool get_next(T& item);
    
    /**
     * Close the buffer.
     * Wakes up all waiting threads. After closing, put operations
     * are still allowed, but get_next will return false once all
     * expected blocks are consumed.
     */
    void close();
    
    /**
     * Check if the buffer is closed.
     * @return true if closed, false otherwise
     */
    bool is_closed() const;

private:
    size_t capacity_;
    std::vector<std::optional<T>> buffer_;
    std::vector<bool> ready_;
    std::vector<size_t> slot_to_index_;  // Track which index is stored in each slot
    size_t next_expected_index_;
    mutable std::mutex mutex_;
    std::condition_variable writer_condition_;  // Notifies writer when next block is ready
    std::condition_variable worker_condition_;   // Notifies workers when space is available
    bool closed_;
    
    /**
     * Check if there's space available for a new block at the given index.
     * Space is available if the index is within the current window
     * (next_expected_index_ to next_expected_index_ + capacity_ - 1).
     */
    bool has_space_for(size_t index) const;
    
    /**
     * Get the current window start (next_expected_index_).
     */
    size_t window_start() const;
    
    /**
     * Get the current window end (next_expected_index_ + capacity_ - 1).
     */
    size_t window_end() const;
    
    /**
     * Convert a global index to a buffer slot index using modulo arithmetic.
     * This allows the buffer to act as a circular buffer.
     */
    size_t index_to_slot(size_t index) const;
};

#include "reorder_buffer.cpp"

#endif // REORDER_BUFFER_HPP

