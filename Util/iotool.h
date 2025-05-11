#ifndef IOTOOL_H
#define IOTOOL_H

#include <unistd.h>
#include <string>
#include <optional>

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
ssize_t do_read(int fd, char* buf, ssize_t bytes_expected);

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
ssize_t read_until_delimiter(int fd, std::string& buf, ssize_t chunk_size, const std::string& delim);

/**
 * @brief Write buffer, retrying on EINTR/EAGAIN.
 *
 * @param fd File descriptor to write to.
 * @param buf Data to write.
 * @param bytes_expected Number of bytes from buf to write.
 * @return Total bytes written (buf + delimiter),
 *         -1 on error, or 0 on unexpected EOF.
 */
ssize_t do_write(int fd, const char* buf, ssize_t bytes_expected);

/**
 * @brief Parse next command from string buffer by extracting up to delim, returning and clearing through delim
 *
 * @param buf string buffer to extract command from
 * @param delim delim to parse on
 * @return command up to delim if present, nullopt if none
 */
std::optional<std::string> parse_next_command(std::string& buf, const std::string& delim);

#endif