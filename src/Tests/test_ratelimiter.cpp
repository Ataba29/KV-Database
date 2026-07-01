#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include "../Limit/Limiter.h"

// Test 1: Normal requests from different IPs are allowed
TEST(RateLimiterTests, AllowsNormalTraffic)
{
    RateLimiter limiter(10, 5, 1000);

    EXPECT_TRUE(limiter.isAllowed("192.168.1.1"));
    EXPECT_TRUE(limiter.isAllowed("192.168.1.2"));
    EXPECT_TRUE(limiter.isAllowed("192.168.1.3"));
}

// Test 2: Single IP gets blocked after exceeding token bucket
TEST(RateLimiterTests, BlocksSingleIPAfterLimit)
{
    // 3 tokens max, 0 refill rate so tokens never come back
    RateLimiter limiter(3, 0, 1000);

    const std::string ip = "10.0.0.1";

    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_TRUE(limiter.isAllowed(ip));

    // 4th request should be blocked
    EXPECT_FALSE(limiter.isAllowed(ip));
}

// Test 3: Blocked IP doesn't affect other IPs
TEST(RateLimiterTests, BlockedIPDoesNotAffectOthers)
{
    RateLimiter limiter(2, 0, 1000);

    const std::string blockedIP = "10.0.0.1";
    const std::string otherIP = "10.0.0.2";

    limiter.isAllowed(blockedIP);
    limiter.isAllowed(blockedIP);
    EXPECT_FALSE(limiter.isAllowed(blockedIP));

    // Other IP should still be fine
    EXPECT_TRUE(limiter.isAllowed(otherIP));
}

// Test 4: Global limit blocks all IPs once reached
TEST(RateLimiterTests, GlobalLimitBlocksAllIPs)
{
    // Global limit of 3 total requests
    RateLimiter limiter(100, 5, 3);

    EXPECT_TRUE(limiter.isAllowed("10.0.0.1"));
    EXPECT_TRUE(limiter.isAllowed("10.0.0.2"));
    EXPECT_TRUE(limiter.isAllowed("10.0.0.3"));

    // 4th request from any IP should be blocked
    EXPECT_FALSE(limiter.isAllowed("10.0.0.4"));
    EXPECT_FALSE(limiter.isAllowed("10.0.0.1"));
}

// Test 5: Tokens refill over time
TEST(RateLimiterTests, TokensRefillOverTime)
{
    // 2 tokens, refill rate of 10/sec
    RateLimiter limiter(2, 10, 1000);

    const std::string ip = "10.0.0.1";

    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_FALSE(limiter.isAllowed(ip)); // bucket empty

    // Wait 500ms — should have refilled ~5 tokens
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_TRUE(limiter.isAllowed(ip)); // should be allowed again
}

// Test 6: Concurrent requests from same IP are handled safely
TEST(RateLimiterTests, HandlesConcurrentRequestsSafely)
{
    RateLimiter limiter(100, 50, 10000);

    const std::string ip = "10.0.0.1";
    std::atomic<int> allowed{0};
    std::atomic<int> blocked{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 50; i++)
    {
        threads.emplace_back([&]()
                             {
            if (limiter.isAllowed(ip))
                allowed++;
            else
                blocked++; });
    }

    for (auto &t : threads)
        t.join();

    EXPECT_EQ(allowed.load() + blocked.load(), 50);
    EXPECT_LE(allowed.load(), 100); // never exceeds max tokens
}