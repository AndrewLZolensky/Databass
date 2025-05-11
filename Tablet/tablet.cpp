#include <iostream>
#include "tablet.h"

std::optional<std::vector<char>> Tablet::get(const std::string& row_key, const std::string& col_key) {

    // look up row_key map in tablet
    auto it_row = table.find(row_key);

    // if not found, return nullopt
    if (it_row == table.end()) {
        return std::nullopt;
    }

    // otherwise, retrieve row_key map
    std::unordered_map<std::string, std::vector<char>>& row_map = it_row->second;

    // look up column in row_key map
    auto col_it = row_map.find(col_key);

    // if not found, return nullopt
    if (col_it == row_map.end()) {
        return std::nullopt;
    }
    
    // otherwise return found value
    return col_it->second;
}


bool Tablet::put(const std::string& row_key, const std::string& col_key, const std::vector<char>& bytes) {
    
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

    return true;
}

bool Tablet::del(const std::string& row_key, const std::string& col_key) {

    // lookup row_key in table
    auto row_it = table.find(row_key);

    // if it does not exist, fail
    if (row_it == table.end()) {
        return false;
    }

    // lookup col_key in row_key map
    std::unordered_map<std::string, std::vector<char>>& retrieved_row = row_it->second;
    auto col_it = retrieved_row.find(col_key);

    // if it does not exist, fail
    if (col_it == retrieved_row.end()) {
        return false;
    }

    // if it does exist, remove entry (col_key, val) from row_key map
    retrieved_row.erase(col_it);

    // check if row_key is empty, and if so, delete (row_key, row_map) from table
    if (retrieved_row.empty()) {
        table.erase(row_it);
    }

    // lastly, return true
    return true;
}