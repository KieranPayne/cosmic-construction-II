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
		std::vector<sf::Vector3i> path;
		bool justJumped = false;
		Human();
		sf::Vector2f GetTexCoords(int height);
		void Tick(Planet *planet);
		void FromJson(nlohmann::json &j);
		nlohmann::json ToJson();

	};
}