#include <iostream>
#include <string>
#include "RAM/HashMap.h" // Includes your custom HashMap from its folder

int main()
{
    std::cout << "=== Initializing Hash Map ===" << std::endl;
    // Create a HashMap with a small capacity (e.g., 5) to force some collisions
    // and thoroughly test your linked list separate chaining logic!
    HashMap phoneBook(5);

    std::cout << "\n=== Testing Insertion ===" << std::endl;
    phoneBook.insert("Alice", "555-1111");
    phoneBook.insert("Bob", "555-2222");
    phoneBook.insert("Charlie", "555-3333");
    std::cout << "Inserted Alice, Bob, and Charlie successfully." << std::endl;

    std::cout << "\n=== Testing Lookup (get) ===" << std::endl;
    std::cout << "Alice's Number: " << phoneBook.get("Alice") << std::endl;
    std::cout << "Bob's Number:   " << phoneBook.get("Bob") << std::endl;

    std::cout << "\n=== Testing Duplicate/Update Logic ===" << std::endl;
    std::cout << "Updating Alice's number..." << std::endl;
    phoneBook.insert("Alice", "555-9999"); // This should update her existing node
    std::cout << "Alice's New Number: " << phoneBook.get("Alice") << std::endl;

    std::cout << "\n=== Testing Missing Key Logic ===" << std::endl;
    std::string missingResult = phoneBook.get("John");
    if (missingResult == "")
    {
        std::cout << "John not found (Correct: Returned an empty string)." << std::endl;
    }
    else
    {
        std::cout << "Error: Found a number for John: " << missingResult << std::endl;
    }

    std::cout << "\n=== Testing Deletion (remove) ===" << std::endl;
    std::cout << "Removing Bob..." << std::endl;
    phoneBook.remove("Bob");

    std::string bobResult = phoneBook.get("Bob");
    if (bobResult == "")
    {
        std::cout << "Bob successfully deleted (Correct: Returned an empty string)." << std::endl;
    }
    else
    {
        std::cout << "Error: Bob still exists with number: " << bobResult << std::endl;
    }

    std::cout << "\n=== Testing Memory Cleanup ===" << std::endl;
    std::cout << "Exiting program. Destructor will now automatically run..." << std::endl;

    return 0;
}