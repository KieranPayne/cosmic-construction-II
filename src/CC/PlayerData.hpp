#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "Serializer.hpp"
namespace cc
{
    class PlayerData
    {
        public:
        PlayerData()
        {
            username = "";
            cameraPosition = {0.f,0.f};
            cameraZoom = 1.f;
        }
        std::string username;
        sf::Vector2f cameraPosition;
        float cameraZoom;
        void Serialize(Serializer& s)
        {
            s.field("username",username);
            s.field("cameraPosition",cameraPosition);
            s.field("cameraZoom",cameraZoom);
        }
    };
}