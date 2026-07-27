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
#include <functional>


/**
 *
 * Definetly didn't look this up
 *
 */
struct SessionKey {
    uint32_t raw_ip;
    uint16_t raw_port;

    bool operator==(const SessionKey& other) const {
        return raw_ip == other.raw_ip && raw_port == other.raw_port;
    }
};

struct SessionKeyHasher {
    std::size_t operator()(const SessionKey& key) const {
        return (std::hash<uint32_t>()(key.raw_ip)) ^ (std::hash<uint16_t>()(key.raw_port) << 1);
    }
};


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
     * * @param client_addr is just the client address
     * @param socket The network socket associated with the session.
     * @param max_idle The maximum duration a session can remain idle before expiration (defaults to 240 seconds).
     */
    SessionKey add_session(SocketType socket, const sockaddr_in& client_addr, std::chrono::seconds max_idle = std::chrono::seconds(240));


    /**
     * @brief Resets the idle timer for a specific user session to the current time.
     * * @param key is just the key that we got from init.
     */
    void update_activity(const SessionKey& key);


    /**
     * @brief Explicitly removes and terminates a specific user session.
     * * @param key is just the key that we got from init.
     */
    void remove_session(const SessionKey& key);


    /**
     * @brief Iterates through all active sessions and terminates those that have exceeded their idle limit.
     */
    void cleanup_expired_sessions();

    /**
     * @brief Forcefully terminates and clears all active user sessions.
     */
    void close_all_sessions();

    /**
     * @brief Registers a callback fired when a session expires from idle timeout.
     * @param cb Invoked with the session's socket, before the session is erased.
     */
    void set_on_session_expired(std::function<void(SocketType)> cb);

private:

    /** @brief Fired from cleanup_expired_sessions() so Server can tear down the connection. */
    std::function<void(SocketType)> on_session_expired;

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
         * @param client_addr is just the client address
         */
        UserSession(SocketType s, const sockaddr_in& client_addr, std::chrono::seconds max_idle);

        /**
         * @brief Destroys the UserSession object.
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
         * @brief Releases session state. Does not close the socket -
         * Server owns socket lifetime and closes it via closeConnection().
         */
        void terminate_session();


        /**
         * Just some Getters for the ip and source port
         * @return
         */
        const std::string& get_ip_str() const { return cached_ip_str; }
        const std::string& get_port_str() const { return cached_port_str; }

        /**
         * @brief Gets the socket owned by this session.
         * @return The session's socket.
         */
        SocketType get_socket() const { return user_socket; }

    private:
        SocketType user_socket;                                  /**< The network socket for this specific user. */
        std::chrono::steady_clock::time_point last_activity_time; /**< Timestamp of the last recorded user activity. */
        std::chrono::seconds session_maximum_idle_time;          /**< Maximum allowed idle duration before expiration. */
        std::string cached_ip_str;
        std::string cached_port_str;
    };

    /** @brief Maps user IP addresses + Source IP to their respective unique session instances. */
    std::unordered_map<SessionKey, std::unique_ptr<UserSession>, SessionKeyHasher> users_sessions;
    /** @brief Shared mutex to ensure thread-safe operations on the sessions map. */
    mutable std::shared_mutex mutex;
};

#endif //KV_DATABASE_USERSESSION_H