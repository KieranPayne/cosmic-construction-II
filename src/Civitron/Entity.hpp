#pragma once
#include "../PCH.hpp"
#include "../json.hpp"

namespace Civitron
{
	enum EntityClass : uint16_t
	{
		NONE = 0,
		HUMAN,
		ITEM
	};
	class Planet;
	class Entity
	{
	public:
		sf::Vector2f position;
		EntityClass myClass = NONE;
		sf::Vector2i size = {1,1};
		Planet *planet;
		bool toBeDeleted = false;
		Entity();
		virtual void Tick(Planet *planet);
		virtual sf::Vector2f GetTexCoords();
		virtual std::vector<sf::Vertex> GetVertices();
		virtual nlohmann::json ToJson();
		virtual void FromJson(nlohmann::json &j);
		virtual void MoveTo(sf::Vector2f newPos);
		static Entity *LoadFromJson(nlohmann::json &j);
	};
}