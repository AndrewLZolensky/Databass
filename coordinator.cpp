#include <iostream>
#include <unordered_map>
#include <stdio.h>
#include <string.h>
#include <bits/stdc++.h>
#include "Util/iotool.h"
#include "Util/admin.h"

/**
 * @brief max number of threads
 */
#define MAX_THREADS 100

/**
 * @brief buffer size for networked read/write
 */
#define BUF_SIZE 1024 * 1024

/**
 * @brief struct to encapsulate thread args
 */
struct arg_st {
    int thread_index;
};

/**
 * @brief array to hold socks
 */
volatile int socks[MAX_THREADS] = {0};

/**
 * @brief optional debug flag to print debug messages, parse as command line arg
 */
bool debug = false;

/**
 * @brief var to store number of server groups to partition servers into
 */
size_t num_groups;

/**
 * @brief vector where index i holds ip, port of server on line i of ipconfig file
 */
std::vector<struct sockaddr_in> index_to_server;

/**
 * @brief map from letters of the alphabet to group number for servers holding the data
 */
std::unordered_map<char, size_t> letter_to_groupno;

/**
 * @brief vector where index i holds a vector servers in group i, stored as indices into index_to_server
 */
std::vector<std::vector<size_t>> groupno_to_indices;

/**
 * @brief Parse and execute command according to protocol (delim was already parsed out)
 *
 * RFC:
 * 
 * @param command to execute
 * @return response
 */
std::string execute_command(const std::string& command, bool& exit_flag) {

    // TODO: if HEARTBEAT...

    // TODO: if LOOKUP <row>
    std::stringstream ss(command);

    // parse method
    std::string method;
    std::getline(ss, method, ' ');
    if (ss.fail()) {
        return "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n-550 Parser Failure";
    }

    // check if exit
    if (method == "EXIT") {
        exit_flag = true;
        return "+OK";
    }

    // check if lookup
    if (method == "LOOKUP") {

        // parse row
        std::string row;
        std::getline(ss, row, ' ');
        if (ss.fail()) {
            return "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n-550 Parser Failure";
        }

        // get first char of row key
        char key = row.c_str()[0];

        // lookup groupno for char
        size_t groupno = letter_to_groupno[key];

        // lookup available server in groupno
        struct sockaddr_in open_srvr = index_to_server[groupno_to_indices[groupno][0]]; // TODO: round-robin

        // respond with available server
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(open_srvr.sin_addr), buf, INET_ADDRSTRLEN);
        std::string ip_str(buf);
        std::string response = std::string("+OK ") + ip_str + std::string(" ") + std::to_string(ntohs(open_srvr.sin_port));
        return response;
    }

    return "-BAD";
}

/**
 * @brief vector where index i holds primary for group i, stored as index into index_to_server
 */
std::vector<size_t> groupno_to_primary;

void* thread_fn(void* args) {

    // cast, copy, and free input args
    arg_st arg = *((arg_st*) args);
    free(args);

    // print message
    fprintf(stderr, "[COORDINATOR] [%d] Accepted new connection\n", arg.thread_index);

    // set prompt
    std::string prompt = "DataStore% ";
    
    // set delim
    std::string delim = "\r\n";

    // init exit flag
    bool exit_flag = true;

    // loop reading commands from stdin
    std::string permbuf;
    std::string tempbuf;
    while (true) {

        // write prompt
        do_write(socks[arg.thread_index], prompt.data(), prompt.size());

        // read until delimiter detected in stream
        ssize_t num_read = read_until_delimiter(socks[arg.thread_index], tempbuf, BUF_SIZE, delim); // clears tempbuf!

        // add read contents to permanent buffer
        permbuf += tempbuf;

        // parse available commands from permanent buffer until there are none
        while (true) {

            // parse next command
            auto command_opt = parse_next_command(permbuf, delim); // if command found, removes from permanent buffer!

            // if there are no more commands, break out of execution loop
            if (!command_opt.has_value()) {
                break;
            }

            // get command
            std::string command = command_opt.value();
            std::string response = execute_command(command, exit_flag);

            // send response
            response += "\n";
            do_write(socks[arg.thread_index], response.c_str(), response.size());

            // if exit flag was set, break execution look
            if (exit_flag) {
                if (debug) {
                    fprintf(stderr, "[%d] Exiting\n", arg.thread_index);
                }
                break;
            }
        }

        // if exit flag was set, break read loop
        if (exit_flag) {
            break;
        }
    }

    // close this connection
    close(socks[arg.thread_index]);

    // zero this thread index into socks
    socks[arg.thread_index] = 0;

    return NULL;
}

/**
 * @brief coordinator main
 * May recieve a PING <index> or LOOKUP <row>, both delimited with \r\n
 * Startup: ./coordinator -v -g <num_groups> -c <ip_config_file_path.txt>
 */
int main(int argc, char* argv[]) {

    // parse path to ipconfig file and debug flag from command line args
    std::string ip_config_file;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            if (argv[i+1]) {
                ip_config_file = argv[i+1];
            } else {
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            debug = true;
        }
        else if (strcmp(argv[i], "-g") == 0) {
            num_groups = std::atoi(argv[i+1]);
        }
    }
    if (debug) {
        fprintf(stderr, "[COORDINATOR] Parsing ips at (%s) for server with (%ld) groups\n", ip_config_file.c_str(), num_groups);
    }

    // parse coordinator address from ipconfig file
    struct sockaddr_in coordinator_addr;
    bool parse_succ = parse_ip_config_at(ip_config_file, coordinator_addr, 0);
    if (!parse_succ) {
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Failed to parse ip config file at 0\n");
        }
        exit(EXIT_FAILURE);
    }
    if (debug) {
        fprintf( stderr, "[COORDINATOR] Parsed coordinator ip and port (%d)\n", ntohs(coordinator_addr.sin_port));
    }

    // get ip, port for all other servers from ip config file <server number, <ip, port>>
    parse_succ = parse_ip_config_except(ip_config_file, index_to_server, 0);
    if (!parse_succ) {
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Failed to parse ip config file except 0\n");
        }
        exit(EXIT_FAILURE);
    }
    if (debug) {
        for (size_t i = 0; i < index_to_server.size(); i++) {
            auto& el = index_to_server.at(i);
            if (debug) {
                fprintf(stderr, "[COORDINATOR] Parsed server ip and port (%d) on line (%ld)\n", ntohs(el.sin_port), i);
            }
        }
    }

    // given nodes, sort into groups <group number, server numbers in group>
    bool group_servers_succ = group_servers(num_groups, index_to_server, groupno_to_indices);
    if (!group_servers_succ) {
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Failed to group servers\n");
        }
        exit(EXIT_FAILURE);
    }
    if (debug) {
        fprintf(stderr, "[COORDINATOR] Successfully grouped servers\n");
    }

    // create mapping from alphabet to groups (fill letter_to_groupno)
    bool map_alphabet_succ = map_alphabet(num_groups, letter_to_groupno);
    if (!map_alphabet_succ) {
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Failed to group servers\n");
        }
        exit(EXIT_FAILURE);
    }
    if (debug) {
        fprintf(stderr, "[COORDINATOR] Successfully mapped alphabet\n");
    }

    // elect primary in each group (fill groupno_to_primary)
    bool elect_primary_succ = elect_primary(groupno_to_indices, groupno_to_primary);
    if (!elect_primary_succ) {
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Failed to group servers\n");
        }
        exit(EXIT_FAILURE);
    }
    if (debug) {
        fprintf(stderr, "[COORDINATOR] Successfully elected primaries\n");
    }

    // create coordinator socket
    int coordinator_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (coordinator_socket < 0) {
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Failed to open server socket\n");
        }
        exit(EXIT_FAILURE);
    }
    if (debug) {
        fprintf(stderr, "[COORDINATOR] Successfully opened socket\n");
    }

    // make socket reusable
    int opt = 1;
	int ret = setsockopt(coordinator_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret < 0) {
        if (debug) {
	        fprintf(stderr, "[COORDINATOR] Error setting socket options (%s)\n", strerror(errno));
        }
	    exit(EXIT_FAILURE);
	}

    // bind coordinator socket to address
    if (bind(coordinator_socket, (struct sockaddr*) &coordinator_addr, sizeof(coordinator_addr)) < 0) {
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Failed to bind server socket to server address\n");
        }
        close(coordinator_socket);
        exit(EXIT_FAILURE);
    }
    if (debug) {
        fprintf(stderr, "[COORDINATOR] Successfully bound socket\n");
    }

    // TODO: notify servers of primary status

    // listen on socket
    listen(coordinator_socket, MAX_THREADS);
    if (debug) {
        fprintf(stderr, "[COORDINATOR] Successfully started listing\n");
    }

    // loop accepting connections
    while (true) {

        // init address struct for incoming client connection
        struct sockaddr_in client_address;
        bzero(&client_address, sizeof(client_address));
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Waiting for connection\n");
        }

        // loop through socks array until one becomes available
        int next_thread_index = 0;
        while (socks[next_thread_index] != 0) {
            next_thread_index++;
            if (next_thread_index >= MAX_THREADS) {
                next_thread_index = 0;
            }
        }
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Found available socket slot\n");
        }

        // accept incoming connection
        socklen_t client_addr_size = sizeof(client_address);
        int client_socket = accept(coordinator_socket, (struct sockaddr*) &client_address, &client_addr_size);
        if (client_socket < 0) {
            if (debug) {
                fprintf(stderr, "[COORDINATOR] Failed to accept connection from client\n");
            }
            close(coordinator_socket);
            exit(EXIT_FAILURE);
        }
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Accepted incoming connection\n");
        }

        // store connection in socks array
        socks[next_thread_index] = client_socket;

        // set up args for handler thread
        arg_st* args = (arg_st*) malloc(sizeof(arg_st));
        args->thread_index = next_thread_index;
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Ready to spawn thread to handle connection\n");
        }

        // spawn handler thread for connection
        pthread_t new_thd;
        int thread_id = pthread_create(&new_thd, NULL, thread_fn, args);
        if (thread_id != 0) {
            if (debug) {
                fprintf(stderr, "[COORDINATOR] Failed to create thread to handle client connection: (%d)\n", errno);
            }
            close(coordinator_socket);
            close(client_socket);
            free(args);
            socks[next_thread_index] = 0;
            exit(EXIT_FAILURE);
        }
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Spawned thread with index (%d) to handle new connection\n", thread_id);
        }

        // detach handler thread for connection
        if (pthread_detach(new_thd) != 0) {
            if (debug) {
                fprintf(stderr, "[COORDINATOR] Failed to detach thread to handle client connection: (%d)\n", errno);
            }
            close(coordinator_socket);
            close(client_socket);
            free(args);
            socks[next_thread_index] = 0;
            exit(EXIT_FAILURE);
        }
        if (debug) {
            fprintf(stderr, "[COORDINATOR] Detached thread with index (%d) to handle new connection\n", thread_id);
        }

    }

    // close server socket
    close(coordinator_socket);

    exit(EXIT_SUCCESS);
}