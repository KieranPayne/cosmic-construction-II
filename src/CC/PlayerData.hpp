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
            planet = 0;
        } 
        std::string username;
        sf::Vector2f cameraPosition;
        float cameraZoom;
        int planet;
        void Serialize(Serializer& s)
        {
            s.field("username",username);
            s.field("cameraPosition",cameraPosition);
            s.field("cameraZoom",cameraZoom);
            s.field("planet",planet);
        }
    };
}