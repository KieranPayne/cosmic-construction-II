#pragma once
#include "../PCH.hpp"
namespace cc
{
    enum class CSMessageType : uint16_t
    {
        SEND_USERNAME = 0,

        JOIN_DATA,

        PLAYER_JOINED,

        PLAYER_LEFT,

        SERVER_MESSAGE,

        CHAT_MESSAGE,

        STATE_CHECK,

        BROADCAST_NEXT_TICK
    };
}