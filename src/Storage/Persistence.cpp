#include "Persistence.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <vector>

Persistence::Persistence()
{
    // Open the AOF file in append mode
    aof_stream.open(aof_path, std::ios::out | std::ios::app);

    if (!aof_stream.is_open())
    {
        throw std::runtime_error("[PERSISTENCE] Error opening AOF file\n");
    }
}

Persistence::~Persistence()
{
    if (aof_stream.is_open())
    {
        aof_stream.flush(); // Force any lingering data in the buffer out to the SSD/HDD
        aof_stream.close();
    }
}

void Persistence::loadDataOnStartup(HashMap &hashmap)
{
    // Load data from RDB
    std::filesystem::path targetPath(rdb_path);

    // Explicitly checks if the file is present on disk
    if (!std::filesystem::exists(targetPath))
    {
        std::cout << "[PERSISTENCE] Path not found.\n";
        return;
    }
    std::ifstream rdb_stream(rdb_path);
    if (!rdb_stream.is_open())
    {
        throw std::runtime_error("[PERSISTENCE] Error opening RDB file\n");
    }
    std::string line;
    while (std::getline(rdb_stream, line))
    {
        std::istringstream iss(line);
        std::string command, key, value;
        iss >> command >> key >> value;
        hashmap.insert(key, value);
    }
    rdb_stream.close();

    // Load data from AOF
    std::filesystem::path targetPath2(aof_path);

    // Explicitly checks if the file is present on disk
    if (!std::filesystem::exists(targetPath2))
    {
        std::cout << "[PERSISTENCE] Path not found.\n";
        return;
    }

    std::ifstream aof_read_stream(aof_path);
    if (!aof_read_stream.is_open())
    {
        throw std::runtime_error("[PERSISTENCE] Error opening AOF file\n");
    }
    while (std::getline(aof_read_stream, line))
    {
        std::istringstream iss(line);
        std::string command, key, value;
        iss >> command;
        if (command == "INSERT")
        {
            iss >> key >> value;
            hashmap.insert(key, value);
        }
        else
        {
            iss >> key;
            hashmap.remove(key);
        }
    }
    aof_read_stream.close();
}

void Persistence::appendToLog(const std::string &command, const std::string &key, const std::string &value)
{
    aof_stream << command + " " + key + " " + value << std::endl;
}

void Persistence::createSnapshot(const HashMap &hashmap)
{
    std::ofstream rdb_write_stream(rdb_path, std::ios::out | std::ios::trunc);
    if (!rdb_write_stream.is_open())
    {
        throw std::runtime_error("[PERSISTENCE] Error opening RDB file for snapshot\n");
    }
    // For each key-value pair that are found inside the hashmap write into the rdb INSERT KEY VALUE\n
    hashmap.forEach([&rdb_write_stream](const std::string &key, const std::string &value)
                    { rdb_write_stream << "INSERT " + key + " " + value << std::endl; });
    rdb_write_stream.close();
}