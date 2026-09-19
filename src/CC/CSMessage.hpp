#pragma once
#include "../PCH.hpp"
namespace cc
{
    //types of messages sent between clients and servers
    enum class CSMessageType
    {
        REQUEST_ADD_ENTITY,
        REQUEST_SET_TILE,
        REQUEST_REMOVE_ENTITY,
        REQUEST_UPDATE_ENTITY,
        REQUEST_GET_CHUNK,
        REQUEST_UPDATE_PLAYER_DATA,

        ADD_ENTITY,
        SET_TILE,
        REMOVE_ENTITY,
        UPDATE_ENTITY,
        SEND_CHUNK,
        UPDATE_PLAYER_DATA,

        REQUEST_RESULT
    };
    class CSMessage
    {
        public:
        CSMessageType type;
        protected:
        void ToPacket(sf::Packet& packet);
        void FromPacket(sf::Packet& packet);
    };
    void CSMessageToPacket(CSMessage* message, sf::Packet& packet);
    CSMessage* CSMessageFromPacket(sf::Packet& packet);
}