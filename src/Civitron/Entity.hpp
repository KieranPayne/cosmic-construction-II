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
		sf::Vector3i position;
		EntityClass myClass = NONE;
		sf::Vector3i size = {1,1,1};
		Planet *planet;
		bool toBeDeleted = false;
		Entity();
		virtual void Tick(Planet *planet);
		virtual sf::Vector2f GetTexCoords(int height);
		virtual std::vector<sf::Vertex> GetVertices(Planet *planet, int height);
		virtual nlohmann::json ToJson();
		virtual void FromJson(nlohmann::json &j);
		virtual void MoveTo(sf::Vector3i newPos);
		static Entity *LoadFromJson(nlohmann::json &j);
	};
}