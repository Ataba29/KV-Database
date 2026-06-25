#include "HashMap.h"
#include <iostream>

HashMap::Node::Node(std::string k, std::string v)
{
    this->key = k;
    this->value = v;
    this->next = nullptr; // New Nodes always start as a null pointer
}

HashMap::HashMap(int size)
{
    this->capacity = size;
    this->table = new Node *[capacity];

    // Initialize all buckets
    for (size_t i = 0; i < capacity; i++)
    {
        table[i] = nullptr;
    }
}

HashMap::~HashMap()
{
    // Go over all buckets
    for (size_t i = 0; i < capacity; i++)
    {
        // For each bucket need to delete the entire linked list
        Node *curr = table[i];
        while (curr != nullptr)
        {
            Node *next = curr->next;
            delete curr;
            curr = next;
        }
    }
    delete[] table; // Finally delete the entire table once its empty
}

int HashMap::hashFunction(std::string key)
{
    int p = 31;
    unsigned long long hashValue = 0;

    for (char c : key)
    {
        // The hash function chose is H = 31 * H-1 + C
        hashValue = (p * hashValue) + c;
    }

    // Make sure not go beyond the scope
    return hashValue % capacity;
}

void HashMap::insert(std::string key, std::string value)
{
    int hashValue = hashFunction(key);
    Node *curr = table[hashValue];
    while (curr != nullptr)
    {
        if (curr->key == key)
        {
            curr->value = value; // Update the phone number
            return;
        }
        curr = curr->next; // Move to the next node
    }
    Node *newNode = new Node(key, value);
    newNode->next = table[hashValue]; // Aim at the start of the linked list
    table[hashValue] = newNode;       // Start the chain at the new Node
}

std::string HashMap::get(std::string key)
{
    int hashValue = hashFunction(key);
    Node *curr = table[hashValue];
    while (curr != nullptr)
    {
        if (curr->key == key)
        {
            return curr->value;
        }
        curr = curr->next; // Move to the next node
    }
    // Key was not Found
    return "";
}

void HashMap::remove(std::string key)
{
    int hashValue = hashFunction(key);
    Node *curr = table[hashValue];
    Node *prev = nullptr;
    while (curr != nullptr)
    {
        if (curr->key == key)
        {
            // Delete head
            if (prev == nullptr)
            {
                table[hashValue] = curr->next;
            }
            // Delete in chain
            else
            {
                prev->next = curr->next;
            }
            delete curr;
            return;
        }
        prev = curr;
        curr = curr->next; // Move to the next node
    }
    // If this line was reached then there is nothing to delete
}