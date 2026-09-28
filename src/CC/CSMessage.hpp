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
        CHAT_MESSAGE
    };
}