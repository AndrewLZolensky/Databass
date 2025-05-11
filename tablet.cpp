#include <iostream>
#include "tablet.h"

std::optional<std::vector<char>> Tablet::get(const std::string& row_key, const std::string& col_key) {

    //acquire admin lock
    if (pthread_mutex_lock(&admin) != 0) {
        return std::nullopt;
    }

    // find lock
    auto it_lock = locks.find(row_key);
    
    // if lock dne, return
    if (it_lock == locks.end()) {
        pthread_mutex_unlock(&admin);
        return std::nullopt;
    }

    // otherwise acquire row lock
    pthread_mutex_t& row_lock = it_lock->second;
    pthread_mutex_lock(&row_lock);

    // release admin lock
    pthread_mutex_unlock(&admin);

    // look up row_key map in tablet
    auto it_row = table.find(row_key);

    // if not found, return nullopt
    if (it_row == table.end()) {
        pthread_mutex_unlock(&row_lock);
        return std::nullopt;
    }

    // otherwise, retrieve row_key map
    std::unordered_map<std::string, std::vector<char>>& row_map = it_row->second;

    // look up column in row_key map
    auto col_it = row_map.find(col_key);

    // if not found, return nullopt
    if (col_it == row_map.end()) {
        pthread_mutex_unlock(&row_lock);
        return std::nullopt;
    }

    // copy value to return
    std::vector<char> ret = col_it->second;

    // release row lock
    pthread_mutex_unlock(&row_lock);
    
    // otherwise return found value
    return ret;
}


bool Tablet::put(const std::string& row_key, const std::string& col_key, const std::vector<char>& bytes) {

    // acquire admin lock
    if (pthread_mutex_lock(&admin) != 0) {
        return false;
    }

    // acquire lock for row
    auto lock_it = locks.find(row_key);

    // if lock does not exist, create it
    if (lock_it == locks.end()) {
        
        // Insert a default-constructed mutex first
        auto insert_result = locks.emplace(row_key, pthread_mutex_t{});
        
        // Initialize the mutex in-place in the map
        if (pthread_mutex_init(&(insert_result.first->second), NULL) != 0) {
            // If initialization fails, remove it from the map
            locks.erase(insert_result.first);
            pthread_mutex_unlock(&admin);
            return false;
        }
        
        lock_it = insert_result.first;
    }

    // acquire row lock
    pthread_mutex_t& row_lock = lock_it->second;
    if (pthread_mutex_lock(&row_lock) != 0) {
        pthread_mutex_unlock(&admin);
        return false;
    }

    // release admin lock
    pthread_mutex_unlock(&admin);
    
    // lookup row_key in database
    auto row_it = table.find(row_key);
    
    // if row_key does not exist, create it
    if (row_it == table.end()) {
        table.emplace(row_key, std::unordered_map<std::string, std::vector<char>>());
        row_it = table.find(row_key);
    }

    // if row_key was not created successfully, fail
    if (row_it == table.end()) {
        return false;
    }

    // set entry for col_key
    std::unordered_map<std::string, std::vector<char>>& retrieved_row = row_it->second;
    retrieved_row[col_key] = bytes;

    // release row lock
    pthread_mutex_unlock(&row_lock);

    return true;
}

bool Tablet::del(const std::string& row_key, const std::string& col_key) {

    // acquire admin lock and hold it until end
    if (pthread_mutex_lock(&admin) != 0) {
        return false;
    }

    // acquire row lock and hold it until delete
    auto lock_it = locks.find(row_key);
    
    // if lock dne, release admin lock and return false
    if (lock_it == locks.end()) {
        pthread_mutex_unlock(&admin);
        return false;
    }

    // otherwise acquire lock
    pthread_mutex_t& row_lock = lock_it->second;
    if (pthread_mutex_lock(&row_lock) != 0) {
        pthread_mutex_unlock(&admin);
        return false;
    }

    // lookup row_key in table
    auto row_it = table.find(row_key);

    // if it does not exist, fail
    if (row_it == table.end()) {
        pthread_mutex_unlock(&admin);
        pthread_mutex_unlock(&row_lock);
        return false;
    }

    // lookup col_key in row_key map
    std::unordered_map<std::string, std::vector<char>>& retrieved_row = row_it->second;
    auto col_it = retrieved_row.find(col_key);

    // if it does not exist, fail
    if (col_it == retrieved_row.end()) {
        pthread_mutex_unlock(&admin);
        pthread_mutex_unlock(&row_lock);
        return false;
    }

    // if it does exist, remove entry (col_key, val) from row_key map
    retrieved_row.erase(col_it);

    // check if row_key is empty
    if (retrieved_row.empty()) {

        // if so, delete row from table
        table.erase(row_it);

        // then, release row lock
        pthread_mutex_unlock(&row_lock);

        // then, destroy row lock
        pthread_mutex_destroy(&row_lock);

        // then, erase row lock from map
        locks.erase(row_key);
    } else {

        // otherwise, release row lock
        pthread_mutex_unlock(&row_lock);
    }

    // release admin lock
    pthread_mutex_unlock(&admin);

    // lastly, return true
    return true;
}