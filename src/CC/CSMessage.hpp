#pragma once
#include "../PCH.hpp"
namespace cc
{
    //types of messages sent between clients and servers
    enum class CSMessageType : uint16_t
    {
        SEND_USERNAME,
        JOIN_DATA,
        PLAYER_JOINED,
        PLAYER_LEFT,
        SERVER_MESSAGE,
        CHAT_MESSAGE,
        REQUEST_CHUNKS,
        CHUNK_DATA, //result from requesting chunks
        REQUEST_SET_TILES,
        SET_TILES,
        REQUEST_ADD_ENTITIES,
        ADD_ENTITIES,
        REQUEST_UPDATE_ENTITIES,
        UPDATE_ENTITIES,
        UPDATE_PLAYER_DATA,
    };
}