//
// Created by razi on 7/3/2026.
//

#ifndef KV_DATABASE_USERSESSIONBACKGROUNDWORKER_H
#define KV_DATABASE_USERSESSIONBACKGROUNDWORKER_H
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include "../UserSession/UserSession.h"

class UserSessionBackgroundWorker {
public:
    UserSessionBackgroundWorker(UserSessionManager& userSessionManager, std::chrono::minutes intervalMinutes = std::chrono::minutes(1));
    ~UserSessionBackgroundWorker();

private:
    UserSessionManager& user_sessions;
    std::thread background_thread;
    std::atomic<bool> active;               /**< Controls whether the scheduler is running. */
    std::chrono::minutes interval;          /**< How often to check the userSessions. */
    std::condition_variable condition_variable;   /**< Bell that wakes up sleeping Thread */
    std::mutex mutex;

    /**
     * This is function that will be excecuted by the thread.
     */
    void run();
};

#endif //KV_DATABASE_USERSESSIONBACKGROUNDWORKER_H
