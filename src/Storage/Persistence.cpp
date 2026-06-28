#include "Persistence.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <thread>

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
    // Load data from RDB (only if it exists)
    std::filesystem::path targetPath(rdb_path);
    if (std::filesystem::exists(targetPath))
    {
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
    }
    else
    {
        std::cout << "[PERSISTENCE] No RDB snapshot found, skipping.\n";
    }

    // Load data from AOF (only if it exists)
    std::filesystem::path targetPath2(aof_path);
    if (!std::filesystem::exists(targetPath2))
    {
        std::cout << "[PERSISTENCE] No AOF log found, skipping.\n";
        return;
    }

    std::ifstream aof_read_stream(aof_path);
    if (!aof_read_stream.is_open())
    {
        throw std::runtime_error("[PERSISTENCE] Error opening AOF file\n");
    }
    std::string line;
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
    std::lock_guard<std::mutex> lock(aof_mutex);
    aof_stream << command + " " + key + " " + value << std::endl;
}

void Persistence::createSnapshot(const HashMap &hashmap)
{
    // Write current hashmap state to RDB snapshot file
    std::ofstream rdb_write_stream(rdb_path, std::ios::out | std::ios::trunc);
    if (!rdb_write_stream.is_open())
    {
        throw std::runtime_error("[PERSISTENCE] Error opening RDB file for snapshot\n");
    }

    hashmap.forEach([&rdb_write_stream](const std::string &key, const std::string &value)
                    { rdb_write_stream << "INSERT " + key + " " + value << std::endl; });

    rdb_write_stream.close();

    // Reset AOF log since snapshot now represents the full database state
    std::lock_guard<std::mutex> lock(aof_mutex);
    aof_stream.flush();
    aof_stream.close();
    std::filesystem::resize_file(aof_path, 0);
    aof_stream.open(aof_path, std::ios::out | std::ios::app);

    if (!aof_stream.is_open())
    {
        throw std::runtime_error("[PERSISTENCE] Error opening AOF file after reset\n");
    }

    std::cout << "[PERSISTENCE] Snapshot complete, AOF reset.\n";
}