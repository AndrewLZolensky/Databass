#include <iostream>
#include <unordered_map>
#include "Util/iotool.h"

/**
 * @brief coordinator main
 * May recieve a PING <index> or LOOKUP <row>, both delimited with \r\n
 * Startup: ./coordinator -v -p <ip_config_file_path.txt>
 */
int main(int argc, char* argv[]) {

    // TODO: parse path to ipconfig file from command line args
    const std::string ip_config_file = "./Res/ipconfig.txt";

    // parse coordinator ip, port from ipconfig file
    std::string ip;
    int port;
    bool parse_succ = parse_ip_config_at(ip_config_file, ip, port, 0);
    if (!parse_succ) {
        exit(EXIT_FAILURE);
    }

    // get ip, port for all other servers from ip config file
    std::unordered_map<int, struct server> server_table;
    parse_succ = parse_ip_config_full(ip_config_file, server_table);

    // TODO: given nodes, sort into groups

    // TODO: create mapping from alphabet to groups

    // TODO: elect primary in each group

    // TODO: listen on socket

    // TODO: if PING...

    // TODO: if LOOKUP <row>
    exit(EXIT_SUCCESS);
}