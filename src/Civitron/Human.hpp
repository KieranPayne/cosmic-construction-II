#pragma once
#include "Entity.hpp"
#include "ItemData.hpp"
#include <queue>
namespace Civitron
{
	class Human : public Entity
	{
	public:
		std::vector<ItemData> inventory;
		Human();
		sf::Vector2f GetTexCoords();
		void Tick(Planet *planet);
		void FromJson(nlohmann::json &j);
		nlohmann::json ToJson();

	};
}