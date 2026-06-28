#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

/**
 * @brief A fixed-size thread pool that manages concurrent job execution.
 *
 * Maintains a pool of worker threads that sleep when idle and wake up
 * when new jobs are submitted to the queue.
 */
class ThreadPool
{
private:
    std::queue<std::function<void()>> jobs; /**< Queue of pending jobs to be executed. */
    std::vector<std::thread> threads;       /**< Pool of worker threads. */
    std::mutex m;                           /**< Mutex protecting the job queue. */
    std::condition_variable cv;             /**< Bell that wakes up sleeping workers. */
    std::atomic<bool> active;               /**< Controls whether the pool is running. */

public:
    /**
     * @brief Constructs the thread pool and spawns worker threads.
     * @param size The number of worker threads to create (default: 3).
     */
    ThreadPool(int size = 3);

    /**
     * @brief Destroys the thread pool, waits for all threads to finish.
     */
    ~ThreadPool();

    /**
     * @brief Submits a new job to the queue for execution.
     * @param job A callable with no arguments or return value.
     */
    void acceptJob(std::function<void()> job);
};

#endif