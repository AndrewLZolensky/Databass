#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "Util/iotool.h"
#include "Tablet/tablet.h"

/**
 * @brief max number of threads
 */
#define MAX_THREADS 100

/**
 * @brief buffer size for networked read/write
 */
#define BUF_SIZE 1024 * 1024

/**
 * @brief array to hold socks
 */
volatile int socks[MAX_THREADS] = {0};

/**
 * @brief struct to encapsulate thread args
 */
struct arg_st {
    int thread_index;
};

/**
 * @brief main tablet to store data
 */
Tablet tab;

/**
 * @brief port for server to run on
 */
int port;

/**
 * @brief ip for server to run on
 */
std::string ip;

/**
 * @brief debug message flag
 */
bool debug;

/**
 * @brief Parse and execute command according to protocol (delim was already parsed out)
 *
 * RFC:
 *  GET:
 *   CMD: GET <row> <col>
 *   RSP: 250 OK <bytes>, 550 FAILURE
 *  PUT:
 *   CMD: PUT <row> <col> <bytes>
 *   RSP: 250 OK, 550 FAILURE
 *  DEL:
 *   CMD: DEL <row> <col>
 *   RSP: 250 OK, 550 FAILURE
 * 
 * @param command to execute
 * @return response
 */
std::string execute_command(const std::string& command, bool& exit_flag) {

    // prepare command for parsing
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
        return "+950 GOODBYE";
    }

    // parse row
    std::string row;
    std::getline(ss, row, ' ');
    if (ss.fail()) {
        return "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n-550 Parser Failure";
    }

    // parse col
    std::string col;
    std::getline(ss, col, ' ');
    if (ss.fail()) {
        return "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n-550 Parser Failure";
    }

    // branch on method
    if (method == "PUT") {

        // parse bytes;
        std::string bytes_str;
        bytes_str = command.substr(method.size() + row.size() + col.size() + 3);
        std::vector<char> bytes_vec(bytes_str.begin(), bytes_str.end());

        // execute PUT
        bool put_succ = tab.put(row, col, bytes_vec);
        if (!put_succ) {
            return "-550 Resource Creation Failed";
        }

        // respond
        return "+250 OK";

    } else if (method == "GET") {

        // execute get
        auto gotten_opt = tab.get(row, col);
        if (!gotten_opt.has_value()) {
            return "-550 Resource Does Not Exist";
        }

        // unpack data
        std::vector<char> gotten = gotten_opt.value();

        // format response
        std::string formatted_data(gotten.begin(), gotten.end());

        // respond
        return "+250 OK " + formatted_data;

    } else if (method == "DEL") {

        // execute delete
        bool del_succ = tab.del(row, col);
        if (!del_succ) {
            return "-550 Resource Does Not Exist";
        }

        // respond
        return "+250 OK";

    }

    // if invalid use
    return "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n-550 Parser Failure";
}

void* thread_fn(void* args) {

    // cast, copy, and free input args
    arg_st arg = *((arg_st*) args);
    free(args);

    // print message
    fprintf(stderr, "[%d] Accepted new connection\n", arg.thread_index);

    // set prompt
    std::string prompt = "DataStore% ";
    
    // set delim
    std::string delim = "\r\n";

    // init exit flag ->> NOW: TRUE ->> CONNECTIONLESS
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
 * Startup: ./server -v -n <ip_config_file_line_nbr>
 */
int main(int argc, char* argv[]) {

    // TODO: parse server number and debug from command line args

    // TODO: parse ip config file to get own ip and coordinator ip
    ip = "0.0.0.0";

    // parse server port and whether to run in debug mode
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            if (argv[i+1]) {
                port = std::atoi(argv[i+1]);
            } else {
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-i") == 0) {
            if (argv[i+1]) {
                ip = argv[i+1];
            } else {
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            debug = true;
        }
    }

    // print parsed inputs
    if (debug) {
        fprintf(stderr, "Parsed server ip, port: %s, %d\n", ip.c_str(), port);
    };

    // create socket
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        if (debug) {
            fprintf(stderr, "Failed to open server socket\n");
        }
        exit(EXIT_FAILURE);
    }

    // make socket reusable
    int opt = 1;
	int ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret < 0) {
	    fprintf(stderr, "Error setting socket options (%s)\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}

    // set up server address struct
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &(server_address.sin_addr)) <= 0) {
        if (debug) {
            fprintf(stderr, "Failed to fill sockaddr_in subfield from ip string\n");
        }
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // bind server socket to server address
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        if (debug) {
            fprintf(stderr, "Failed to bind server socket to server address\n");
        }
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // listen on socket
    listen(server_socket, MAX_THREADS);

    // TODO: set up heartbeats to coordinator

    // loop accepting connections
    while (true) {

        // TODO: periodically send heatbeats to coordinator

        // init address struct for incoming client connection
        struct sockaddr_in client_address;
        bzero(&client_address, sizeof(client_address));

        // loop through socks array until one becomes available
        int next_thread_index = 0;
        while (socks[next_thread_index] != 0) {
            next_thread_index++;
            if (next_thread_index >= MAX_THREADS) {
                next_thread_index = 0;
            }
        }

        // accept incoming connection
        socklen_t client_addr_size = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_addr_size);
        if (client_socket < 0) {
            if (debug) {
                fprintf(stderr, "Failed to accept connection from client\n");
            }
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // store connection in socks array
        socks[next_thread_index] = client_socket;

        // set up args for handler thread
        arg_st* args = (arg_st*) malloc(sizeof(arg_st));
        args->thread_index = next_thread_index;

        // spawn handler thread for connection
        pthread_t new_thd;
        int thread_id = pthread_create(&new_thd, NULL, thread_fn, args);
        if (thread_id != 0) {
            if (debug) {
                fprintf(stderr, "Failed to create thread to handle client connection: (%d)\n", errno);
            }
            close(server_socket);
            close(client_socket);
            free(args);
            socks[next_thread_index] = 0;
            exit(EXIT_FAILURE);
        }

        // detach handler thread for connection
        if (pthread_detach(new_thd) != 0) {
            fprintf(stderr, "Failed to detach thread to handle client connection: (%d)\n", errno);
            close(server_socket);
            close(client_socket);
            free(args);
            socks[next_thread_index] = 0;
            exit(EXIT_FAILURE);
        }

    }

    // close server socket
    close(server_socket);

    // exit
    exit(EXIT_SUCCESS);
}