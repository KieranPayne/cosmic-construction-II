#pragma once
#include "Entity.hpp"
#include "ItemData.hpp"
namespace Civitron{
    class Item : public Entity{
        public:
        ItemData data;
        Item();
        sf::Vector2f GetTexCoords();
		void Tick(Planet *planet);

        void FromJson(nlohmann::json& j);
        nlohmann::json ToJson();
    };   
}