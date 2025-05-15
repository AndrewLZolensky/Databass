#include "admin.h"
#include <cstdlib>
#include <ctime>

bool group_servers(size_t num_groups, std::vector<struct sockaddr_in>& index_to_server, std::vector<std::vector<size_t>>& groupno_to_indices) {

    // bounds check
    if (num_groups == 0) {
        return false;
    }

    // initialize indices vectors
    groupno_to_indices.clear();
    groupno_to_indices.resize(num_groups);

    // fill indices vectors in round-robin fashion
    for (size_t i = 0; i < index_to_server.size(); i++) {
        size_t group_no = i % num_groups;
        groupno_to_indices[group_no].push_back(i);
    }
    return true;
}

bool map_alphabet(size_t num_groups, std::unordered_map<char, size_t>& letter_to_groupno) {

    // bounds check
    if (num_groups == 0) {
        return false;
    }

    // assign groups to chars in a round-robin fashion
    for (int i = 0; i < 256; i++) {
        char temp(i - 128);
        letter_to_groupno[temp] = i % num_groups;
    }
    return true;
}

bool elect_primary_for_group(size_t groupno, std::vector<std::vector<size_t>>& groupno_to_indices, std::vector<size_t>& groupno_to_primary) {

    // bounds check
    if (groupno >= groupno_to_indices.size()) {
        return false;
    }

    // elect primary by selecting random server from nodes in group
    size_t min = 0;
    size_t max = groupno_to_indices[groupno].size();
    size_t random = rand() % (max - min) + min; // random index into groupno_to_indices[groupno] (vector of group's node ids)
    groupno_to_primary[groupno] = groupno_to_indices[groupno][random]; // set primary to id at random index
    return true;
}

bool elect_primary(std::vector<std::vector<size_t>>& groupno_to_indices, std::vector<size_t>& groupno_to_primary) {

    // ensure groupno_to_primary is correct size
    groupno_to_primary.clear();
    groupno_to_primary.resize(groupno_to_indices.size());

    // seed random number generator
    srand(time(0));

    // for each group, elect a random primary (for now: later, will add alive check)
    for (size_t i = 0; i < groupno_to_indices.size(); i++) {
        elect_primary_for_group(i, groupno_to_indices, groupno_to_primary);
    }

    return true;
}