#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <string>
#include <fstream>
#include "../RAM/Hashmap.h"

/**
 * @brief Manages the disk storage layer for the Key-Value database.
 *
 * This class handles hybrid durability by combining a fast, real-time Append-Only File (AOF)
 * log with periodic point-in-time Snapshotting (RDB). It guarantees data recovery
 * across server restarts and crashes.
 */
class Persistence
{
private:
    std::ofstream aof_stream;                      /**< Persistent file stream handle for real-time logging. */
    const std::string aof_path = "appendonly.log"; /**< File path for the transaction log file. */
    const std::string rdb_path = "snapshot.log";   /**< File path for the point-in-time snapshot file. */

public:
    /**
     * @brief Construct a new Persistence object.
     *
     * Initializes file handles and ensures that the storage environment
     * is ready for reading or writing.
     */
    Persistence();

    /**
     * @brief Destroy the Persistence object.
     *
     * Safely closes open file streams and flushes remaining buffers to disk.
     */
    ~Persistence();

    /**
     * @brief Restores the database state during system boot.
     *
     * Reads the `snapshot.rdb` file first to populate the bulk dataset, then
     * replays any outstanding transactions recorded in `appendonly.aof` to bring
     * the memory state up to date.
     *
     * @param hashmap A reference to the in-memory HashMap to be populated.
     */
    void loadDataOnStartup(HashMap &hashmap);

    /**
     * @brief Logs a mutating write operation to disk immediately.
     *
     * Appends state-changing operations (like SET or DEL) to the transaction log file.
     * This provides immediate durability for incoming write requests.
     *
     * @param command The operation type executed (e.g., "SET", "DEL").
     * @param key The database key targeted by the operation.
     * @param value The database value associated with the key (empty string for deletes).
     */
    void appendToLog(const std::string &command, const std::string &key, const std::string &value);

    /**
     * @brief Takes a full point-in-time snapshot of the database.
     *
     * Serializes the current, entire contents of the in-memory HashMap and
     * overrides the existing `snapshot.rdb` file with this new baseline state.
     *
     * @note This operation can be resource-heavy and should ideally be scheduled or offloaded.
     * @param hashmap A constant reference to the in-memory HashMap data source.
     */
    void createSnapshot(const HashMap &hashmap);
};

#endif