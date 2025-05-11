#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <optional>
#include <sstream>
#include "Tablet/tablet.h"
#include "Util/iotool.h"

#define BUF_SIZE 1024 * 1024

Tablet tab;

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
std::string execute_command(const std::string& command) {

    // prepare command for parsing
    std::stringstream ss(command);

    // parse method
    std::string method;
    std::getline(ss, method, ' ');
    if (ss.fail()) {
        fprintf(stderr, "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n");
        return "-550 Parser Failure";
    }

    // parse row
    std::string row;
    std::getline(ss, row, ' ');
    if (ss.fail()) {
        fprintf(stderr, "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n");
        return "-550 Parser Failure";
    }

    // parse col
    std::string col;
    std::getline(ss, col, ' ');
    if (ss.fail()) {
        fprintf(stderr, "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n");
        return "-550 Parser Failure";
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
    fprintf(stderr, "1) GET <row> <col>\n2) PUT <row> <col> <bytes>\n3) DEL <row> <col>\n");
    return "-550 Parser Failure";
}

int main(int argc, char** argv) {

    // set prompt
    std::string prompt = "DataStore % ";

    // loop reading commands from stdin
    std::string permbuf;
    std::string tempbuf;
    while (true) {

        // write prompt
        do_write(STDOUT_FILENO, prompt.data(), prompt.size());

        // read command
        ssize_t num_read = read_until_delimiter(STDIN_FILENO, tempbuf, BUF_SIZE, "\n"); // clears tempbuf!

        // add to permanent buffer
        permbuf += tempbuf;

        // parse available commands and continue
        while (true) {
            auto command_opt = parse_next_command(permbuf, "\n"); // if command found, removes from permanent buffer!
            if (!command_opt.has_value()) {
                break;
            }
            std::string command = command_opt.value();
            std::string response = execute_command(command);
            response += "\n";
            do_write(STDOUT_FILENO, response.c_str(), response.size());
        }
    }

    // exit
    exit(EXIT_SUCCESS);
}