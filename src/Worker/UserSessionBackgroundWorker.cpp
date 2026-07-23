//
// Created by razi on 7/3/2026.
//

#include "UserSessionBackgroundWorker.h"
#include <iostream>

UserSessionBackgroundWorker::UserSessionBackgroundWorker(UserSessionManager& user_session_manager, std::chrono::minutes interval_in_minutes)
    : user_sessions(user_session_manager), interval(interval_in_minutes), active(true)
{
    background_thread = std::thread(&UserSessionBackgroundWorker::run, this);
}
UserSessionBackgroundWorker::~UserSessionBackgroundWorker() {
    active = false;
    condition_variable.notify_all();

    if (background_thread.joinable()) {
        background_thread.join();
    }
}


void UserSessionBackgroundWorker::run() {
    while (this->active) {
        std::unique_lock lock(this->mutex);
        this->condition_variable.wait_for(lock, this->interval, [this] { return !active; });

        // The thread woke up
        if (!active) {
            break;
        }
        std::cout << "[USBM] Running looking for idle connections" << std::endl;
        lock.unlock();
        this->user_sessions.cleanup_expired_sessions();
    }
}


