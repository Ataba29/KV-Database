#include <iostream>
#include "ThreadPool.h"

ThreadPool::ThreadPool(int size)
{
    active = true;
    for (int i = 0; i < size; i++)
    {
        threads.push_back(std::thread([this]()
                                      {
            while (true)
            {
                std::function<void()> job;

                {
                    // Lock the queue so only one thread touches it at a time
                    std::unique_lock<std::mutex> lock(m);

                    // Sleep until there is a job OR we are shutting down
                    // Releases the lock while sleeping, reacquires it when woken up
                    cv.wait(lock, [this]() { return !jobs.empty() || !active; });

                    // If we are shutting down and no jobs left, kill this thread
                    if (!active && jobs.empty())
                        return;

                    // Grab the next job from the front of the queue
                    job = jobs.front();

                    // Remove it from the queue so no other thread grabs it
                    jobs.pop();

                } // Lock is released here automatically

                // Run the job OUTSIDE the lock so other threads can grab jobs simultaneously
                job();
            } }));
    }
}

ThreadPool::~ThreadPool()
{
    active = false;
    cv.notify_all();
    for (std::thread &t : threads)
    {
        t.join();
    }
}

void ThreadPool::acceptJob(std::function<void()> job)
{
    std::lock_guard<std::mutex> lock(m);
    jobs.push(job);
    cv.notify_one();
}