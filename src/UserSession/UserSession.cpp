//
// Created by razi on 7/3/2026.
//

#include "UserSession.h"

#include <mutex>

SessionKey UserSessionManager::add_session(SocketType socket, const sockaddr_in& client_addr, std::chrono::seconds max_idle){
    SessionKey key;
    key.raw_ip = client_addr.sin_addr.s_addr;
    key.raw_port = client_addr.sin_port;

    {
        std::shared_lock read_lock(this->mutex);
        if (this->users_sessions.find(key) != this->users_sessions.end()) {
            return key; // Session already exists
        }
    }

    std::unique_lock lock(this->mutex); // Write Lock
    this->users_sessions[key] = std::make_unique<UserSession>(socket, client_addr, max_idle);

    return key;
}

void UserSessionManager::update_activity(const SessionKey& key) {
    std::unique_lock lock(this->mutex); // Write Lock

    if (auto it = users_sessions.find(key); it != users_sessions.end()) {
        it->second->update_last_activity_time();
    }
}

void UserSessionManager::remove_session(const SessionKey& key) {
    std::unique_lock lock(this->mutex); // Write lock
    // *** First time using unique_ptr  not sure if automatically calls the UserSession destructor or not ***
    users_sessions.erase(key);
}

void UserSessionManager::cleanup_expired_sessions() {
    std::unique_lock lock(this->mutex); // Write Lock

    for (auto it = users_sessions.begin(); it != users_sessions.end(); ) {
        if (it->second->is_expired()) {
            // Notify Server before erasing so it can tear down
            if(on_session_expired)
                on_session_expired(it->second->get_socket());
            it = users_sessions.erase(it); // Terminate Will call Automatically
        } else {
            ++it;
        }
    }
}

void UserSessionManager::close_all_sessions() {
    std::unique_lock lock(this->mutex); // Write Lock
    this->users_sessions.clear();
}


void UserSessionManager::set_on_session_expired(std::function<void(SocketType)> cb) {
    on_session_expired = std::move(cb);
}


// --- UserSession Implementation ---

UserSessionManager::UserSession::UserSession(SocketType socket, const sockaddr_in& client_addr, std::chrono::seconds max_idle)
    : user_socket(socket), session_maximum_idle_time(max_idle)
{
    last_activity_time = std::chrono::steady_clock::now();

    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ipBuffer, sizeof(ipBuffer));

    this->cached_ip_str = ipBuffer;
    this->cached_port_str = std::to_string(ntohs(client_addr.sin_port));
}

UserSessionManager::UserSession::~UserSession() {
    terminate_session();
}

void UserSessionManager::UserSession::update_last_activity_time() {
    last_activity_time = std::chrono::steady_clock::now();
}

void UserSessionManager::UserSession::terminate_session() {
    /**  Socket closure is Server's responsibility (see closeConnection()).
    Closing it here would race Server's connections/eventLoop bookkeeping. */

    
    // CloseSocket(user_socket);
}

bool UserSessionManager::UserSession::is_expired() const {
    return (std::chrono::steady_clock::now() - last_activity_time) > this->session_maximum_idle_time;
}