//
// Created by razi on 7/3/2026.
//

#ifndef KV_DATABASE_USERSESSION_H
#define KV_DATABASE_USERSESSION_H


#include "../Networking/NetworkTypes.h"
#include <chrono>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class UserSessionManager {


public:
    UserSessionManager() = default;
    ~UserSessionManager() = default;

    void add_session(const std::string& ip, SocketType socket, std::chrono::seconds max_idle = std::chrono::seconds(60));
    void update_activity(const std::string& ip);
    void remove_session(const std::string& ip);
    void cleanup_expired_sessions();
    void close_all_sessions();


private:
    class UserSession {
    public:
        UserSession(SocketType s, std::chrono::seconds max_idle);
        ~UserSession();

        bool is_expired() const;
        void update_last_activity_time();
        void terminate_session();

    private:
        SocketType user_socket;
        std::chrono::steady_clock::time_point last_activity_time;
        std::chrono::seconds session_maximum_idle_time;
    };

    std::unordered_map<std::string, std::unique_ptr<UserSession>> mapping_ip_to_session;
    mutable std::shared_mutex mutex;
};

#endif //KV_DATABASE_USERSESSION_H
