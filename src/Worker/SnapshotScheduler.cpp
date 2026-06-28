#include "SnapshotScheduler.h"
#include <iostream>

SnapshotScheduler::SnapshotScheduler(std::function<void()> snapshotCallback, int intervalMinutes)
{
    this->interval = std::chrono::minutes(intervalMinutes);
    this->snapshotCallback = snapshotCallback;
    this->active = true;
    this->schedulerThread = std::thread(&SnapshotScheduler::run, this);
}

SnapshotScheduler::~SnapshotScheduler()
{
    active = false;
    cv.notify_all();
    schedulerThread.join();
}

void SnapshotScheduler::run()
{
    while (active)
    {
        // Sleep for the interval OR until woken up by destructor
        std::unique_lock<std::mutex> lock(m);
        cv.wait_for(lock, interval, [this]()
                    { return !active; });

        if (!active)
            return;

        lock.unlock(); // Release before calling callback

        std::cout << "[SCHEDULER] Taking snapshot...\n";
        snapshotCallback();
        std::cout << "[SCHEDULER] Snapshot complete.\n";
    }
}