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