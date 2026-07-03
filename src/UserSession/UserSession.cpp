//
// Created by razi on 7/3/2026.
//

#include "UserSession.h"

#include <mutex>

void UserSessionManager::add_session(const std::string& ip, SocketType socket, std::chrono::seconds max_idle) {
    {
        std::shared_lock read_lock(this->mutex);
        if (this->mapping_ip_to_session.find(ip) != this->mapping_ip_to_session.end()) {
            return;
        }
    }

    std::unique_lock lock(this->mutex); // Write Lock
    this->mapping_ip_to_session[ip] = std::make_unique<UserSession>(socket, max_idle);
}

void UserSessionManager::update_activity(const std::string& ip) {
    std::unique_lock lock(this->mutex); // Write Lock

    if (auto it = mapping_ip_to_session.find(ip); it != mapping_ip_to_session.end()) {
        it->second->update_last_activity_time();
    }
}

void UserSessionManager::remove_session(const std::string& ip) {
    std::unique_lock lock(this->mutex); // Write lock
    // *** First time using unique_ptr  not sure if automatically calls the UserSession destructor or not ***
    mapping_ip_to_session.erase(ip);
}

void UserSessionManager::cleanup_expired_sessions() {
    std::unique_lock lock(this->mutex); // Write Lock

    for (auto it = mapping_ip_to_session.begin(); it != mapping_ip_to_session.end(); ) {
        if (it->second->is_expired()) {
            it = mapping_ip_to_session.erase(it); // Terminate Will call Automatically
        } else {
            ++it;
        }
    }
}

void UserSessionManager::close_all_sessions() {
    std::unique_lock lock(this->mutex); // Write Lock
    this->mapping_ip_to_session.clear();
}


UserSessionManager::UserSession::UserSession(SocketType socket, std::chrono::seconds max_idle)
    : user_socket(socket), session_maximum_idle_time(max_idle)
{
    last_activity_time = std::chrono::steady_clock::now();
}

UserSessionManager::UserSession::~UserSession() {
    terminate_session();
}

void UserSessionManager::UserSession::update_last_activity_time() {
    last_activity_time = std::chrono::steady_clock::now();
}

void UserSessionManager::UserSession::terminate_session() {
    CloseSocket(user_socket);
}

bool UserSessionManager::UserSession::is_expired() const {
    return (std::chrono::steady_clock::now() - last_activity_time) > this->session_maximum_idle_time;
}