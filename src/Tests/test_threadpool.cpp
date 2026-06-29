#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include "../Worker/Threadpool.h"

// Test 1: Basic job execution
TEST(ThreadPoolTests, ExecutesSingleJob)
{
    ThreadPool pool{1};
    std::atomic<bool> jobRan{false};

    pool.acceptJob([&jobRan]()
                   { jobRan = true; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(jobRan);
}

// Test 2: All jobs run exactly once
TEST(ThreadPoolTests, ExecutesAllJobsExactlyOnce)
{
    ThreadPool pool{3};
    const int jobCount = 100;
    std::atomic<int> counter{0};

    for (int i = 0; i < jobCount; i++)
    {
        pool.acceptJob([&counter]()
                       { counter++; });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_EQ(counter.load(), jobCount);
}

// Test 3: Jobs run concurrently not sequentially
TEST(ThreadPoolTests, RunsJobsConcurrently)
{
    ThreadPool pool{3};
    std::atomic<int> peakConcurrency{0};
    std::atomic<int> currentConcurrency{0};
    std::mutex peakMutex;

    auto job = [&]()
    {
        int current = ++currentConcurrency;
        {
            std::lock_guard<std::mutex> lock(peakMutex);
            if (current > peakConcurrency.load())
                peakConcurrency = current;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        --currentConcurrency;
    };

    for (int i = 0; i < 9; i++)
        pool.acceptJob(job);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_GT(peakConcurrency.load(), 1);
}

// Test 4: Pool survives a job that throws an exception
TEST(ThreadPoolTests, SurvivesJobThatThrows)
{
    ThreadPool pool{2};
    std::atomic<int> successCount{0};

    pool.acceptJob([]()
                   { throw std::runtime_error("chaos job"); });
    pool.acceptJob([&successCount]()
                   { successCount++; });
    pool.acceptJob([&successCount]()
                   { successCount++; });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(successCount.load(), 2);
}

// Test 5: Pool handles 1000 jobs with 1 thread
TEST(ThreadPoolTests, SingleThreadHandlesManyJobs)
{
    ThreadPool pool{1};
    std::vector<int> results;
    std::mutex resultsMutex;

    for (int i = 0; i < 1000; i++)
    {
        pool.acceptJob([&results, &resultsMutex, i]()
                       {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results.push_back(i); });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    EXPECT_EQ(results.size(), 1000);
}