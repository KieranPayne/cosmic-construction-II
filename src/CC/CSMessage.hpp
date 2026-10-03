#pragma once
#include "../PCH.hpp"
namespace cc
{
    /**
     * @brief Types of messages sent between clients and servers.
     *
     * Every packet starts with one of these values, written as a uint16, followed by a payload that
     * depends on the type. The direction and payload of each message are described on each value.
     *
     * Conventions used in the payload descriptions:
     *  - "bytes" means a uint64 length followed by that many raw bytes.
     *  - "entity" means a uint16 entity type followed by bytes holding the serialized entity.
     *  - "int" counts are 32-bit.
     *  - Tile positions are world tile coordinates; chunk positions are in chunks.
     *
     * @warning The numeric values follow declaration order and travel over the network. Inserting or
     *          reordering values changes the protocol, so add new types at the end, and make sure the
     *          client and server are running the same version.
     */
    enum class CSMessageType : uint16_t
    {
        /// Client to server. The first message after connecting.
        /// Payload: string username.
        SEND_USERNAME,

        /// Server to client. The reply to SEND_USERNAME, with everything needed to start playing.
        /// Payload: bytes (a serialized list of the joined players: an int count, then each player's data),
        /// then an int chunk count and, for each chunk, int x, int y, bytes (the chunk data),
        /// then a uint64 entity count followed by that many entities.
        JOIN_DATA,

        /// Server to all other clients. Another player has joined.
        /// Payload: bytes (the serialized data of the new player).
        PLAYER_JOINED,

        /// Server to all clients. A player has disconnected.
        /// Payload: string username.
        PLAYER_LEFT,

        /// Server to all clients. A message from the server, shown in the chat log.
        /// Payload: string message.
        SERVER_MESSAGE,

        /// Client to server: string message.
        /// Server to the other clients: string username of the sender, then string message.
        CHAT_MESSAGE,

        /// Client to server. Asks for chunks the client does not have.
        /// Payload: uint64 count, then for each chunk: int x, int y.
        REQUEST_CHUNKS,

        /// Server to client. The result of REQUEST_CHUNKS.
        /// Payload: uint64 count, then for each chunk: int x, int y, bytes (the chunk data).
        CHUNK_DATA,

        /// Client to server. The player has changed tiles.
        /// Payload: int count, then for each tile: int x, int y, uint16 tile type, bool hasTileEntity,
        /// and, only if hasTileEntity is true, bytes (the serialized tile entity).
        REQUEST_SET_TILES,

        /// Server to the other clients on the same planet. Relays REQUEST_SET_TILES.
        /// Payload: same as REQUEST_SET_TILES.
        SET_TILES,

        /// Client to server. The player has added entities.
        /// Payload: int count, then that many entities.
        REQUEST_ADD_ENTITIES,

        /// Server to the other clients on the same planet. Relays REQUEST_ADD_ENTITIES.
        /// Payload: same as REQUEST_ADD_ENTITIES.
        ADD_ENTITIES,

        /// Client to server. The player has changed existing entities.
        /// Payload: int count, then for each: int index (into the planet's entity list), then the new entity.
        REQUEST_UPDATE_ENTITIES,

        /// Server to the other clients on the same planet. Relays REQUEST_UPDATE_ENTITIES.
        /// Payload: same as REQUEST_UPDATE_ENTITIES.
        UPDATE_ENTITIES,

        /// Client to server: bytes (the sender's serialized player data: camera, planet, etc.).
        /// Server to the other clients: string username of the sender, then bytes (their player data).
        UPDATE_PLAYER_DATA,
    };
}