#pragma once
#include "Entity.hpp"
#include "ItemData.hpp"
namespace cc
{
    class Item : public Entity
    {
        public:
        ItemData itemData;
        Item();
        sf::Vector2f GetTexCoords();
        nlohmann::json ToJson();
        void FromJson(nlohmann::json& j);
    };
}