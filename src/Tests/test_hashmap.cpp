#include <gtest/gtest.h>
#include "../RAM/HashMap.h"


TEST(HashMapTests, HandlesInsertCorrectly) {
    HashMap map{ 2 };
    const std::string key = "key";
    const std::string value = "value";

    ASSERT_NO_THROW(map.insert(key, value));
    
    EXPECT_EQ(map.get(key), value);
}

TEST(HashMapTests, HandlesGetWithDuplicateKeyInsertions) {
    HashMap map{ 3 };

    const std::string key = "key";

    const std::string value1 = "value1";
    const std::string value2 = "value2";

    ASSERT_NO_THROW(map.insert(key, value1));

    ASSERT_NO_THROW(map.insert(key, value2));

    EXPECT_EQ(map.get(key), value2);
}


TEST(HashMapTests, HandledRemoveCorrectly) {
    HashMap map{ 2 };
    const std::string key = "key";
    const std::string value = "value";

    ASSERT_NO_THROW(map.insert(key, value));

    ASSERT_NO_THROW(map.remove(key));
    
    EXPECT_EQ(map.get(key), "");
}


TEST(HashMapTests, HandledRemoveWhenKeyIsntInHashmap) {
    HashMap map{ 2 };

    const std::string key = "key";
    const std::string value = "value";
    ASSERT_NO_THROW(map.insert(key, value));

    ASSERT_NO_THROW(map.remove("Random Key"));
    
    EXPECT_EQ(map.get(key), value);
}

TEST(HashMapConcurrencyTests, HandlesConcurrentInsertsSafely) {
    HashMap map{ 5 }; // The map is small to make sure that there is many collitions
    const int num_threads = 5;
    const int inserts_per_thread = 100;
    
    std::vector<std::thread> threads;
	std::atomic<bool> gate{ false };

    auto worker = [&map, &gate](int thread_id, int inserts) {
		while (!gate.load()) {
            // Created a Spinner Lock (Were threads are hot and ready to fire!)
		}
		for (int i{}; i < inserts; ++i) {
			std::string unique_key = "thread_" + std::to_string(thread_id) + "_key_" + std::to_string(i);
			std::string value = "value_" + std::to_string(i);

            map.insert(unique_key, value);
        }
    };

    // At this point we spawn the threads and let them spin
    for (int i{}; i < num_threads; ++i) {
        threads.emplace_back(worker, i, inserts_per_thread);
    }

	// Open the floodgates!
    gate.store(true);

    for (auto& t : threads) {
        t.join();
    }

    for (int t{}; t < num_threads; ++t) {
        for (int i{}; i < inserts_per_thread; ++i) {
            std::string expected_key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
            std::string expected_value = "value_" + std::to_string(i);
            
            EXPECT_EQ(map.get(expected_key), expected_value) 
                << "Missing data from thread " << t << " at iteration " << i;
        }
    }
}
