#ifndef SNAPSHOTSCHEDULER_H
#define SNAPSHOTSCHEDULER_H

#include <thread>
#include <atomic>
#include <chrono>
#include <functional>

/**
 * @brief Runs a background thread that triggers RDB snapshots on a fixed interval.
 */
class SnapshotScheduler
{
private:
    std::thread schedulerThread;            /**< Background thread that runs the scheduler loop. */
    std::atomic<bool> active;               /**< Controls whether the scheduler is running. */
    std::function<void()> snapshotCallback; /**< The snapshot function to call every interval. */
    std::chrono::minutes interval;          /**< How often to take a snapshot. */

    /**
     * @brief The main loop that sleeps then triggers a snapshot.
     */
    void run();

public:
    /**
     * @brief Starts the scheduler with a given snapshot function and interval.
     * @param snapshotCallback The function to call to create a snapshot.
     * @param intervalMinutes How often to snapshot in minutes (default: 5).
     */
    SnapshotScheduler(std::function<void()> snapshotCallback, int intervalMinutes = 5);

    /**
     * @brief Stops the scheduler and joins the background thread.
     */
    ~SnapshotScheduler();
};

#endif