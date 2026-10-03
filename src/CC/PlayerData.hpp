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
            resolution = {1920.f,1080.f};
            username = "";
            cameraPosition = {0.f,0.f};
            cameraZoom = 1.f;
            planet = 0;
        }
        sf::Vector2f resolution;
        std::string username;
        sf::Vector2f cameraPosition;
        float cameraZoom;
        int planet;
        void Serialize(Serializer& s)
        {
            s.field("resolution",resolution);
            s.field("username",username);
            s.field("cameraPosition",cameraPosition);
            s.field("cameraZoom",cameraZoom);
            s.field("planet",planet);
        }
    };
}