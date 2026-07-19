#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

#include "../Limit/Limiter.h"

static uint32_t ipToUint32(const std::string &ip)
{
    in_addr addr{};

    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1)
        throw std::runtime_error("Invalid IPv4 address");

    return ntohl(addr.s_addr);
}

// Test 1: Normal requests from different IPs are allowed
TEST(RateLimiterTests, AllowsNormalTraffic)
{
    RateLimiter limiter(10, 5, 1000);

    EXPECT_TRUE(limiter.isAllowed(ipToUint32("192.168.1.1")));
    EXPECT_TRUE(limiter.isAllowed(ipToUint32("192.168.1.2")));
    EXPECT_TRUE(limiter.isAllowed(ipToUint32("192.168.1.3")));
}

// Test 2: Single IP gets blocked after exceeding token bucket
TEST(RateLimiterTests, BlocksSingleIPAfterLimit)
{
    RateLimiter limiter(3, 0, 1000);

    const uint32_t ip = ipToUint32("10.0.0.1");

    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_TRUE(limiter.isAllowed(ip));

    EXPECT_FALSE(limiter.isAllowed(ip));
}

// Test 3: Blocked IP doesn't affect other IPs
TEST(RateLimiterTests, BlockedIPDoesNotAffectOthers)
{
    RateLimiter limiter(2, 0, 1000);

    const uint32_t blockedIP = ipToUint32("10.0.0.1");
    const uint32_t otherIP = ipToUint32("10.0.0.2");

    limiter.isAllowed(blockedIP);
    limiter.isAllowed(blockedIP);
    EXPECT_FALSE(limiter.isAllowed(blockedIP));

    EXPECT_TRUE(limiter.isAllowed(otherIP));
}

// Test 4: Global limit blocks all IPs once reached
TEST(RateLimiterTests, GlobalLimitBlocksAllIPs)
{
    RateLimiter limiter(100, 5, 3);

    EXPECT_TRUE(limiter.isAllowed(ipToUint32("10.0.0.1")));
    EXPECT_TRUE(limiter.isAllowed(ipToUint32("10.0.0.2")));
    EXPECT_TRUE(limiter.isAllowed(ipToUint32("10.0.0.3")));

    EXPECT_FALSE(limiter.isAllowed(ipToUint32("10.0.0.4")));
    EXPECT_FALSE(limiter.isAllowed(ipToUint32("10.0.0.1")));
}

// Test 5: Tokens refill over time
TEST(RateLimiterTests, TokensRefillOverTime)
{
    RateLimiter limiter(2, 10, 1000);

    const uint32_t ip = ipToUint32("10.0.0.1");

    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_TRUE(limiter.isAllowed(ip));
    EXPECT_FALSE(limiter.isAllowed(ip));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_TRUE(limiter.isAllowed(ip));
}

// Test 6: Concurrent requests from same IP are handled safely
TEST(RateLimiterTests, HandlesConcurrentRequestsSafely)
{
    RateLimiter limiter(100, 50, 10000);

    const uint32_t ip = ipToUint32("10.0.0.1");

    std::atomic<int> allowed{0};
    std::atomic<int> blocked{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < 50; ++i)
    {
        threads.emplace_back([&]()
                             {
            if (limiter.isAllowed(ip))
                ++allowed;
            else
                ++blocked; });
    }

    for (auto &t : threads)
        t.join();

    EXPECT_EQ(allowed.load() + blocked.load(), 50);
    EXPECT_LE(allowed.load(), 100);
}