#include "format_utils.hpp"
#include <sstream>
#include <iomanip>

std::string format_time(double seconds) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    
    if (seconds < 0.001) {
        oss << (seconds * 1000000) << " μs";
    } else if (seconds < 1.0) {
        oss << (seconds * 1000) << " ms";
    } else {
        oss << seconds << " s";
    }
    
    return oss.str();
}

std::string format_size(size_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << (bytes / 1024.0) << " KB";
    } else {
        oss << (bytes / (1024.0 * 1024.0)) << " MB";
    }
    
    return oss.str();
}

