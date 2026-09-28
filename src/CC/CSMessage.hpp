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
        REQUEST_SET_TILE,
        SET_TILE,
        REQUEST_ADD_ENTITY,
        ADD_ENTITY
    };
}