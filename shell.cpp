#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <optional>
#include <sstream>
#include "tablet.h"

#define BUF_SIZE 1024 * 1024

Tablet tab;

/**
 * @brief Read until max bytes, retrying on EINTR.
 *
 * @param fd File descriptor to read from.
 * @param buf Buffer to store read data (must be at least bytes_expected bytes).
 * @param bytes_expected Maximum bytes to read.
 * @param delim Delimiter string to search for.
 * @return Total bytes read (including delimiter if found),
 *         0 on EOF, or -1 on error.
 */
ssize_t do_read(int fd, char* buf, ssize_t bytes_expected) {

    // track bytes read
    ssize_t bytes_read = 0;

    // loop while we have not read the expected number of bytes
    while (bytes_read < bytes_expected) {
        ssize_t temp_num_read = read(fd, buf + bytes_read, bytes_expected - bytes_read);
        if (temp_num_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (temp_num_read == 0) {
            return 0;
        }

        // increment num read
        bytes_read += temp_num_read;
    }

    // if we break on EOF return bytes read
    return bytes_read;
}

/**
 * @brief Read until delimiter, retrying on EINTR. Once finished, data is stored in buf.data()
 *
 * @param fd File descriptor to read from.
 * @param buf Buffer to store read data (must be at least bytes_expected bytes).
 * @param bytes_expected Maximum bytes to read.
 * @param delim Delimiter string to search for.
 * @return Total bytes read (including delimiter if found),
 *         0 on EOF, or -1 on error.
 */
ssize_t read_until_delimiter(int fd, std::string& buf, ssize_t chunk_size, const std::string& delim) {

    // track bytes read
    ssize_t bytes_read = 0;
    char temp_buf[chunk_size];

    // clear buf
    buf.clear();

    // loop while we have not read the expected number of bytes
    while (true) {
        ssize_t temp_num_read = read(fd, temp_buf, chunk_size);
        if (temp_num_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (temp_num_read == 0) {
            return 0;
        }

        // add data to buf
        buf.append(temp_buf, temp_num_read);
        bytes_read += temp_num_read;
        

        // search for delimiter and if found return true
        size_t pos = buf.find(delim);
        if (pos != std::string::npos) {
            break;
        }

    }

    // if we break on EOF return bytes read
    return bytes_read;
}

/**
 * @brief Write buffer, retrying on EINTR/EAGAIN.
 *
 * @param fd File descriptor to write to.
 * @param buf Data to write.
 * @param bytes_expected Number of bytes from buf to write.
 * @return Total bytes written (buf + delimiter),
 *         -1 on error, or 0 on unexpected EOF.
 */
ssize_t do_write(int fd, const char* buf, ssize_t bytes_expected) {

    // write buffer
    ssize_t bytes_written = 0;
    while (bytes_written < bytes_expected) {
        ssize_t temp_num_written = write(fd, buf + bytes_written, bytes_expected - bytes_written);
        if (temp_num_written < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return -1;
        } else if (temp_num_written == 0) {
            return 0;
        }
        bytes_written += temp_num_written;
    }
    return bytes_written;
}

/**
 * @brief Parse command from string buffer by extracting up to delim, returning and clearing through delim
 *
 * @param buf string buffer to extract command from
 * @param delim delim to parse on
 * @return command up to delim if present, nullopt if none
 */
std::optional<std::string> parse_command(std::string& buf, const std::string& delim) {
    
    // ensure delimiter is present, if not return nullopt
    auto it = buf.find(delim);
    if (it == std::string::npos) {
        return std::nullopt;
    }

    // extract portion of buf up to delim
    std::string res = buf.substr(0, it);

    // clear portion of buf up to delim
    buf = buf.substr(it + delim.size());

    // return extract
    return res;
}

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
    std::string prompt = "DataStore% ";

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
            auto command_opt = parse_command(permbuf, "\n"); // if command found, removes from permanent buffer!
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