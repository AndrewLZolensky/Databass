#include <fstream>
#include <sstream>
#include <vector>
#include "iotool.h"

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

bool tokenize(std::vector<std::string>& tokens, std::string& buffer, char delim) {
    std::stringstream ss(buffer);
    std::string token;
    while (std::getline(ss, token, delim)) {
        tokens.push_back(token);
    }
    return true;
}

std::optional<std::string> parse_next_command(std::string& buf, const std::string& delim) {
    
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

bool parse_ip_config_at(const std::string& ip_config_path, struct sockaddr_in& servaddr, size_t line_nbr) {

    // zero servaddr
    bzero(&servaddr, sizeof(servaddr));

    // open ipconfig file
    std::fstream config_file(ip_config_path);
    if (!config_file.is_open()) {
        return false;
    }

    // read lines of ip config file until ix
    size_t curr_line_nbr = 0;
    std::string curr_line;
    bool ix_found;
    while (getline(config_file, curr_line)) {
        if (curr_line_nbr == line_nbr) {
            ix_found = true;
            break;
        }
    }

    // if ix out of bounds, return false
    if (!ix_found) {
        return false;
    }

    // parse found line into ip and port
    std::vector<std::string> tokens;
    bool tokenize_succ = tokenize(tokens, curr_line, ',');
    if (!tokenize_succ) {
        fprintf(stderr, "parse_ip_config_at() failed to tokenize line (%ld) of ipconfigfile\n\n", line_nbr);
        return false;
    }
    if (tokens.size() != 2) {
        fprintf(stderr, "parse_ip_config_at() read incorrect number of chunks at line (%ld) of ipconfigfile\n", line_nbr);
        return false;
    }

    // fill arguments with ip and port
    std::string temp_ip = tokens[0];
    int temp_port = std::atoi(tokens[1].c_str());
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(temp_port);
    if (inet_pton(AF_INET, temp_ip.c_str(), &(servaddr.sin_addr)) <= 0) {
        fprintf(stderr, "parse_ip_config_at() failed to create ip address from line (%ld) of ipconfigfile\n", line_nbr);
        return false;
    }

    // return
    return true;
}

bool parse_ip_config_except(const std::string& ip_config_path, std::vector<struct sockaddr_in>& servers, size_t line_nbr) {

    // open ipconfig file
    std::fstream config_file(ip_config_path);
    if (!config_file.is_open()) {
        return false;
    }

    // read lines of ip config file until ix
    size_t curr_line_nbr = 0;
    std::string curr_line;
    while (getline(config_file, curr_line)) {

        // skip line nbr we want to avoid
        if (curr_line_nbr == line_nbr) {
            curr_line_nbr += 1;
            continue;
        }

        // parse found line into ip and port
        std::vector<std::string> tokens;
        bool tokenize_succ = tokenize(tokens, curr_line, ',');
        if (!tokenize_succ) {
            fprintf(stderr, "parse_ip_config_except() failed to tokenize line (%ld) of ipconfigfile\n", curr_line_nbr);
            return false;
        }
        if (tokens.size() != 2) {
            fprintf(stderr, "parse_ip_config_except() read incorrect number of chunks in line (%ld) of ipconfigfile\n", curr_line_nbr);
            return false;
        }

        // get server ip and port
        std::string temp_ip = tokens[0];
        int temp_port = std::atoi(tokens[1].c_str());

        // create new address
        // std::pair<std::string, int> temp_ip_port(temp_ip, temp_port);
        struct sockaddr_in temp_servaddr;
        bzero(&temp_servaddr, sizeof(temp_servaddr));
        temp_servaddr.sin_family = AF_INET;
        temp_servaddr.sin_port = htons(temp_port);
        if (inet_pton(AF_INET, temp_ip.c_str(), &(temp_servaddr.sin_addr)) <= 0) {
            fprintf(stderr, "parse_ip_config_except() failed to parse ip address on line (%ld) of ipconfigfile\n", curr_line_nbr);
            return false;
        }

        // add server with ip and port to result
        servers.push_back(temp_servaddr);

        // increment line nbr
        curr_line_nbr += 1;
        
    }

    // return
    return true;
}