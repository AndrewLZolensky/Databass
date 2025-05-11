#ifndef tablet_header
#define tablet_header

#include <optional>
#include <vector>
#include <unordered_map>
#include <string>
#include <unistd.h>

/**
 * @class Tablet
 * @brief In‐memory two‐level key→(row, column)→blob store.
 *
 * Uses an outer unordered_map keyed by row, and an inner unordered_map
 * keyed by column, to hold std::vector<char> blobs.
 */
class Tablet {

    /* public methods */
    public:

        Tablet() {
            pthread_mutex_init(&admin, NULL);
        }

        ~Tablet() {
            
            // Destroy all row locks
            for (auto& [key, mutex] : locks) {
                pthread_mutex_destroy(&mutex);
            }
            
            // Destroy the admin lock
            pthread_mutex_destroy(&admin);
        }
        /**
         * @brief Retrieve the byte vector at the specified row and column.
         *
         * Looks up the outer map by @p row_key, then the inner map by @p col_key.
         * If found, returns a copy of the stored std::vector<char>;
         * otherwise returns std::nullopt.
         *
         * @param row_key  The row identifier in the table.
         * @param col_key  The column identifier within that row.
         * @return A std::optional containing the blob if present,
         *         or std::nullopt if the row or column does not exist.
         */
        std::optional<std::vector<char>> get(const std::string& row_key, const std::string& col_key);

        /**
         * @brief Insert or overwrite a blob at the specified row and column.
         *
         * If the row does not exist, it is created.  The blob in @p bytes
         * is copied into the table at (row_key, col_key), replacing any
         * existing data.
         *
         * @param row_key  The row identifier in the table.
         * @param col_key  The column identifier within that row.
         * @param bytes    The data blob to store (copied).
         * @return true if the operation succeeded, false if entry does not exist and entry allocation failed
         */
        bool put(const std::string& row_key, const std::string& col_key, const std::vector<char>& bytes);

        /**
         * @brief Delete the blob at the specified row and column.
         *
         * If the cell exists, removes it and, if that leaves the row empty,
         * also removes the row from the table.
         *
         * @param row_key  The row identifier in the table.
         * @param col_key  The column identifier within that row.
         * @return true if a blob was present and removed, false if resource does not exist
         */
        bool del(const std::string& row_key, const std::string& col_key);
    
    /* private fields */
    private:
        /**
         * @brief Two‐level map: row key → (column key → byte‐vector blob).
         *
         * The outer map’s key is the row identifier.  Each value is another
         * unordered_map whose key is the column identifier and whose value
         * is the stored std::vector<char> blob.
         */
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<char>>> table;

        std::unordered_map<std::string, pthread_mutex_t> locks;

        pthread_mutex_t admin;
};

#endif