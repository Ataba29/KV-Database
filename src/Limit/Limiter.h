#ifndef RATELIMITER_H
#define RATELIMITER_H

#include <unordered_map>
#include <chrono>
#include <mutex>
#include <string>
#include <atomic>

/**
 * @brief Enforces per-IP rate limiting using the Token Bucket algorithm.
 *
 * Each IP gets a bucket of tokens. Each request costs one token.
 * Tokens refill at a fixed rate. Empty bucket = request blocked.
 */
class RateLimiter
{
private:
    /**
     * @brief Token bucket state for a single IP address.
     */
    struct IPRecord
    {
        double tokens;                                    /**< Current token count. */
        std::chrono::steady_clock::time_point lastRefill; /**< Last time tokens were added. */
    };

    std::unordered_map<std::string, IPRecord> ipRecords; /**< Per-IP bucket state. */
    std::mutex mapMutex;                                 /**< Protects ipRecords from concurrent access. */

    std::atomic<int> globalCount;                            /**< Total requests in current global window. */
    std::chrono::steady_clock::time_point globalWindowStart; /**< Start of current global window. */
    std::mutex globalMutex;                                  /**< Protects global window state. */

    double maxTokens;      /**< Maximum bucket capacity per IP. */
    double refillRate;     /**< Tokens added per second per IP. */
    int maxRequestsGlobal; /**< Max total requests per second across all IPs. */
    /**
     * @brief Checks the global rate limit across all clients.
     * @return true if the global request limit allows the request,
     *         false if the global limit has been reached.
     */
    bool isAllowedGlobal();

    /**
     * @brief Checks the per-IP token bucket rate limit.
     * @param ip The client IP address to check.
     * @return true if the IP has available tokens and the request is
     *         allowed, false if the IP is rate limited.
     */
    bool isAllowedPrivate(const std::string &ip);

public:
    /**
     * @brief Constructs the rate limiter.
     * @param maxTokens Max burst capacity per IP (default: 10).
     * @param refillRate Tokens refilled per second per IP (default: 5).
     * @param globalLimit Max total requests per second (default: 1000).
     */
    RateLimiter(double maxTokens = 10, double refillRate = 5, int globalLimit = 1000);

    /**
     * @brief Destroys the rate limiter and clears all state.
     */
    ~RateLimiter();

    /**
     * @brief Checks if a request from the given IP should be allowed.
     * @param ip The client IP address as a string.
     * @return true if allowed, false if rate limited.
     */
    bool isAllowed(const std::string &ip);
};

#endif