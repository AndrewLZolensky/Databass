#ifndef ADMIN_H
#define ADMIN_H

#include <vector>
#include <string>
#include <unordered_map>
#include <arpa/inet.h>

/**
 * @brief assign servers to groups
 * @param num_groups to break servers into
 * @param index_to_server index i contains the sockaddr_in for server i
 * @param groupno_to_indices index i contains a vector of servers in group i, stored as indices into index_to_server
 * @returns true if success, false else
 */
bool group_servers(size_t num_groups, std::vector<struct sockaddr_in>& index_to_server, std::vector<std::vector<size_t>>& groupno_to_indices);

/**
 * @brief map all valid characters to a group no
 * @param num_groups to allocate characters to
 * @param letter_to_groupno unordered map from character to group no to fill with assignment
 * @returns true if success, false else
 */
bool map_alphabet(size_t num_groups, std::unordered_map<char, size_t>& letter_to_groupno);

/**
 * @brief elect a primary from amongst the nodes in group
 * @param groupno to have election for
 * @param groupno_to_indices index i contains a vector of servers in group i, stored as indices into index_to_server
 * @param groupno_to_primary index i will be filled with the primary server for group i, stored as an index into index_to_server
 * @returns true if success, false else
 */
bool elect_primary_for_group(size_t groupno, std::vector<std::vector<size_t>>& groupno_to_indices, std::vector<size_t>& groupno_to_primary);

/**
 * @brief elect a primary from amongst the nodes in each group
 * @param groupno_to_indices index i contains a vector of servers in group i, stored as indices into index_to_server
 * @param groupno_to_primary index i will be filled with the primary server for group i, stored as an index into index_to_server
 * @returns true if success, false else
 */
bool elect_primary(std::vector<std::vector<size_t>>& groupno_to_indices, std::vector<size_t>& groupno_to_primary);

#endif