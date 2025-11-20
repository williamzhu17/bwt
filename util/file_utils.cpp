#include "file_utils.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <algorithm>

bool create_directory(const std::string& dir_path) {
    struct stat st;
    if (stat(dir_path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(dir_path.c_str(), 0755) == 0;
}

bool file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

bool directory_exists(const std::string& dir_path) {
    struct stat st;
    return (stat(dir_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

size_t get_file_size(const std::string& filename) {
    struct stat buffer;
    if (stat(filename.c_str(), &buffer) != 0) {
        return 0;
    }
    return buffer.st_size;
}

std::vector<std::string> list_files_in_directory(const std::string& dir_path) {
    std::vector<std::string> files;
    DIR* dir = opendir(dir_path.c_str());
    
    if (dir == nullptr) {
        std::cerr << "Warning: Could not open directory: " << dir_path << std::endl;
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
            continue;
        }
        
        std::string full_path = dir_path + "/" + entry->d_name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            files.push_back(entry->d_name);
        }
    }
    
    closedir(dir);
    return files;
}

bool files_are_identical(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);
    
    if (!f1.is_open() || !f2.is_open()) {
        return false;
    }
    
    // Compare byte by byte
    char c1, c2;
    while (true) {
        bool has1 = (bool)f1.get(c1);
        bool has2 = (bool)f2.get(c2);
        
        // If both reached EOF at the same time, files are identical
        if (!has1 && !has2) {
            return true;
        }
        
        // If only one reached EOF, files differ in length
        if (!has1 || !has2) {
            return false;
        }
        
        // If bytes differ, files are different
        if (c1 != c2) {
            return false;
        }
    }
}

