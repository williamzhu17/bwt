#include "reorder_buffer.hpp"

template<typename T>
ReorderBuffer<T>::ReorderBuffer(size_t capacity)
    : capacity_(capacity)
    , buffer_(capacity)
    , ready_(capacity, false)
    , slot_to_index_(capacity, std::numeric_limits<size_t>::max())  // Initialize with invalid index
    , next_expected_index_(0)
    , closed_(false)
{
}

template<typename T>
bool ReorderBuffer<T>::has_space_for(size_t index) const {
    // Check if index is within the current window
    return index >= window_start() && index <= window_end();
}

template<typename T>
size_t ReorderBuffer<T>::window_start() const {
    return next_expected_index_;
}

template<typename T>
size_t ReorderBuffer<T>::window_end() const {
    return next_expected_index_ + capacity_ - 1;
}

template<typename T>
size_t ReorderBuffer<T>::index_to_slot(size_t index) const {
    // Use modulo to map global index to buffer slot
    // This creates a circular buffer effect
    return index % capacity_;
}

template<typename T>
void ReorderBuffer<T>::put(size_t index, const T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until there's space for this index in the buffer
    // Space is available when:
    // 1. The index is within the current window, AND
    // 2. The slot for this index is either free or contains this same index
    size_t slot = index_to_slot(index);
    worker_condition_.wait(lock, [this, index, slot] {
        if (closed_) {
            return true;
        }
        // Check if index is in window and slot is available
        bool in_window = has_space_for(index);
        bool slot_available = (slot_to_index_[slot] == std::numeric_limits<size_t>::max() || slot_to_index_[slot] == index);
        return (in_window && slot_available) || closed_;
    });
    
    if (closed_) {
        return;
    }
    
    // Store the item in the appropriate slot
    buffer_[slot] = item;
    ready_[slot] = true;
    slot_to_index_[slot] = index;
    
    // If this is the next expected block, notify the writer
    if (index == next_expected_index_) {
        writer_condition_.notify_one();
    }
}

template<typename T>
bool ReorderBuffer<T>::get_next(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until the next expected block is ready
    size_t next_slot = index_to_slot(next_expected_index_);
    writer_condition_.wait(lock, [this, next_slot] {
        if (closed_) {
            return true;
        }
        return ready_[next_slot] || closed_;
    });
    
    // If closed and no more blocks, return false
    if (closed_ && !ready_[next_slot]) {
        return false;
    }
    
    // Get the next expected block
    if (ready_[next_slot] && buffer_[next_slot].has_value()) {
        item = std::move(*buffer_[next_slot]);
        buffer_[next_slot].reset();
        ready_[next_slot] = false;
        slot_to_index_[next_slot] = std::numeric_limits<size_t>::max();  // Mark slot as free
        
        // Advance to next expected index
        next_expected_index_++;
        
        // Notify workers that space may now be available
        worker_condition_.notify_all();
        
        return true;
    }
    
    return false;
}

template<typename T>
void ReorderBuffer<T>::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    writer_condition_.notify_all();
    worker_condition_.notify_all();
}

template<typename T>
bool ReorderBuffer<T>::is_closed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

