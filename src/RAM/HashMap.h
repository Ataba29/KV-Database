#ifndef HASHMAP_H
#define HASHMAP_H

#include <string>
#include <functional>

/**
 * @brief A custom Hash Map implementation using Separate Chaining.
 *
 * This class maps string keys (e.g., Names) to string values (e.g., Phone Numbers).
 * It handles collisions by maintaining a linked list of Nodes at each bucket index.
 */
class HashMap
{
private:
    /**
     * @brief A node in the hash map's linked list.
     *
     * Stores the key-value pair and a pointer to the next node
     * to handle bucket collisions via separate chaining.
     */
    struct Node
    {
        std::string key;   /**< The unique string key identifier (e.g., Name). */
        std::string value; /**< The string data value associated with the key (e.g., Phone Number). */
        Node *next;        /**< Pointer to the next node in the chain (nullptr if last). */

        /**
         * @brief Constructs a new Node object.
         * @param k The string key.
         * @param v The string value.
         */
        Node(std::string k, std::string v);
    };

    /** * @brief Dynamic array of Node pointers representing the hash table buckets.
     *
     * Since it points to an array of pointers, it requires double pointer syntax (Node**).
     */
    Node **table;

    int capacity; /**< The total size of the table array (number of available buckets). */

    /**
     * @brief Converts a string key into an array index.
     * @param key The string key to be hashed.
     * @return An integer index between 0 and capacity - 1.
     */
    int hashFunction(std::string key);

public:
    /**
     * @brief Constructs a new HashMap object.
     * @param size The total number of buckets in the hash table (default is 100).
     */
    HashMap(int size = 100);

    /**
     * @brief Destructor that safely frees all dynamically allocated memory.
     */
    ~HashMap();

    /**
     * @brief Inserts a key-value pair into the map. Updates the value if the key exists.
     * @param key The unique identifier for the data.
     * @param value The string value associated with the key.
     */
    void insert(std::string key, std::string value);

    /**
     * @brief Retrieves the value associated with a given key.
     * @param key The key to look up.
     * @return The string value, or an empty string "" if the key is not found.
     */
    std::string get(std::string key);

    /**
     * @brief Removes a key-value pair from the map and frees its memory.
     * @param key The key to delete.
     */
    void remove(std::string key);

    /**
     * @brief Iterates over all key-value pairs in the map.
     * @param callback A function to call for each key-value pair.
     */
    void forEach(std::function<void(const std::string &, const std::string &)> callback) const;
};

#endif