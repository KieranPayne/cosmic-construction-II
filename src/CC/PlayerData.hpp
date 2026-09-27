#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
namespace cc
{
    class PlayerData
    {
        std::string username;
        sf::Vector2f cameraPosition;
        float cameraZoom;
    };
}