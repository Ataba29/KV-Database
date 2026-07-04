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

/**
 * @class UserSessionManager
 * @brief Manages active user network sessions, tracking activity and handling timeouts.
 *
 * This class provides a thread-safe mechanism to register, update, and terminate
 * user sessions based on their IP addresses. It tracks idle times to automatically
 * clean up expired connections.
 */
class UserSessionManager {

public:
    /**
     * @brief Default constructor for UserSessionManager.
     */
    UserSessionManager() = default;

    /**
     * @brief Default destructor for UserSessionManager.
     */
    ~UserSessionManager() = default;

    /**
     * @brief Registers a new user session.
     * * @param ip The unique IP address identifying the user.
     * @param socket The network socket associated with the session.
     * @param max_idle The maximum duration a session can remain idle before expiration (defaults to 60 seconds).
     */
    void add_session(const std::string& ip, SocketType socket, std::chrono::seconds max_idle = std::chrono::seconds(60));

    /**
     * @brief Resets the idle timer for a specific user session to the current time.
     * * @param ip The IP address of the user whose activity is being updated.
     */
    void update_activity(const std::string& ip);

    /**
     * @brief Explicitly removes and terminates a specific user session.
     * * @param ip The IP address of the session to remove.
     */
    void remove_session(const std::string& ip);

    /**
     * @brief Iterates through all active sessions and terminates those that have exceeded their idle limit.
     */
    void cleanup_expired_sessions();

    /**
     * @brief Forcefully terminates and clears all active user sessions.
     */
    void close_all_sessions();

private:
    /**
     * @class UserSession
     * @brief Represents an individual user's active session state.
     */
    class UserSession {
    public:
        /**
         * @brief Constructs a new UserSession object.
         * * @param s The network socket assigned to this session.
         * @param max_idle The maximum idle duration permitted for this session.
         */
        UserSession(SocketType s, std::chrono::seconds max_idle);

        /**
         * @brief Destroys the UserSession object, ensuring underlying resources are freed.
         */
        ~UserSession();

        /**
         * @brief Checks whether the session has exceeded its maximum idle time.
         * * @return true If the session has expired.
         * @return false If the session is still active and valid.
         */
        bool is_expired() const;

        /**
         * @brief Updates the internal timestamp to mark the latest user activity.
         */
        void update_last_activity_time();

        /**
         * @brief Gracefully shuts down and terminates the network session.
         */
        void terminate_session();

    private:
        SocketType user_socket;                                  /**< The network socket for this specific user. */
        std::chrono::steady_clock::time_point last_activity_time; /**< Timestamp of the last recorded user activity. */
        std::chrono::seconds session_maximum_idle_time;          /**< Maximum allowed idle duration before expiration. */
    };

    /** @brief Maps user IP addresses to their respective unique session instances. */
    std::unordered_map<std::string, std::unique_ptr<UserSession>> mapping_ip_to_session;

    /** @brief Shared mutex to ensure thread-safe operations on the sessions map. */
    mutable std::shared_mutex mutex;
};

#endif //KV_DATABASE_USERSESSION_H