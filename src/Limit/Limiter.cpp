#include "Limiter.h"

RateLimiter::RateLimiter(double maxTokens, double refillRate, int globalLimit)
{
    this->maxTokens = maxTokens;
    this->refillRate = refillRate;
    this->maxRequestsGlobal = globalLimit;

    this->globalCount = 0;
    this->globalWindowStart = std::chrono::steady_clock::now();
}

RateLimiter::~RateLimiter()
{
    std::lock_guard<std::mutex> lock(mapMutex);
    ipRecords.clear();
}

bool RateLimiter::isAllowedGlobal()
{
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(globalMutex);

    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(
            now - globalWindowStart)
            .count();

    // reset window
    if (elapsed >= 1)
    {
        globalCount = 0;
        globalWindowStart = now;
    }

    // reject if global limit reached
    if (globalCount >= maxRequestsGlobal)
    {
        return false;
    }

    globalCount++;

    return true;
}

bool RateLimiter::isAllowedPrivate(const std::string &ip)
{
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(mapMutex);

    // create bucket for new IP
    auto [it, inserted] =
        ipRecords.try_emplace(ip, IPRecord{maxTokens, now});

    IPRecord &record = it->second;

    // calculate refill amount
    double seconds =
        std::chrono::duration<double>(
            now - record.lastRefill)
            .count();

    record.tokens += seconds * refillRate;

    // don't exceed bucket size
    if (record.tokens > maxTokens)
    {
        record.tokens = maxTokens;
    }

    record.lastRefill = now;

    // no tokens left
    if (record.tokens < 1)
    {
        return false;
    }

    // consume one token
    record.tokens -= 1;

    return true;
}

bool RateLimiter::isAllowed(const std::string &ip)
{
    return isAllowedGlobal() && isAllowedPrivate(ip);
}