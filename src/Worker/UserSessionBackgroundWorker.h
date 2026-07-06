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

/**
 * @class UserSessionBackgroundWorker
 * @brief A background worker that periodically triggers session cleanup tasks.
 *
 * This class spawns a dedicated worker thread that periodically invokes
 * the session manager's expiration cleanup logic.
 */
class UserSessionBackgroundWorker {
public:
    /**
     * @brief Constructs the background worker and starts the execution thread.
     * @param userSessionManager Reference to the session manager instance to clean up.
     * @param intervalMinutes Frequency of the cleanup checks (defaults to 1 minute).
     */
    UserSessionBackgroundWorker(UserSessionManager& userSessionManager, std::chrono::minutes intervalMinutes = std::chrono::minutes(1));

    /**
     * @brief Destructor. Signals the worker thread to stop and joins it safely.
     */
    ~UserSessionBackgroundWorker();

private:
    UserSessionManager& user_sessions;            /**< Reference to the managed user sessions. */
    std::thread background_thread;                /**< The dedicated thread running the cleanup loop. */
    std::atomic<bool> active;                     /**< Flag controlling whether the worker loop is running. */
    std::chrono::minutes interval;                /**< Duration interval between consecutive cleanup checks. */
    std::condition_variable condition_variable;   /**< Condition variable used to wake up the sleeping thread or signal stop. */
    std::mutex mutex;                             /**< Mutex protecting the condition variable state. */

    /**
     * @brief The core loop function executed by the background thread.
     * * Periodically wakes up based on the specified interval to trigger session
     * cleanups, or instantly wakes up when notified to shut down.
     */
    void run();
};

#endif //KV_DATABASE_USERSESSIONBACKGROUNDWORKER_H