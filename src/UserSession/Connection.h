#ifndef CONNECTION_H
#define CONNECTION_H

#include "../Networking/NetworkTypes.h"
#include "UserSession.h"

/**
 * @file Connection.h
 * @brief Per-client connection state tracked by the server.
 *
 * Bundles together everything the server needs to know about a single
 * connected client, keyed by socket in the Server's connection map.
 */

/**
 * @brief Holds the state associated with a single connected client.
 */
struct Connection
{
    /// The client's socket handle.
    SocketType socket;

    /// The session key identifying this client's UserSession entry.
    SessionKey sessionKey;

    /// This is the command of the user (Used for fragmentation management)
    std::string commandBuffer;

    Connection(SocketType sock, SessionKey key)
        : socket(sock), sessionKey(key) {}
};

#endif // CONNECTION_H
