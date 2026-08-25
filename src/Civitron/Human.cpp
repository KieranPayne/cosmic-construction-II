#include "Human.hpp"
#include "Utils.hpp"
#include "EntityInfo.hpp"
#include "Planet.hpp"
#include "TileInfo.hpp"
#include "Pathfinding.hpp"
#include "Item.hpp"
#include <queue>
namespace Civitron
{
	Human::Human()
	{
		inventory = {{0,0}, {0,0}, {0,0}};
		position = {0, 0};
		myClass = HUMAN;
	}
	sf::Vector2f Human::GetTexCoords()
	{
		return JsonAsVector(EntityInfo::texturesJson["Human"]);
	}
	void Human::Tick(Planet *planet)
	{
	}

	nlohmann::json Human::ToJson()
	{
		nlohmann::json j = Entity::ToJson();
		j["inventory"] = {};
		for (auto& i : inventory){
			j["inventory"].push_back(i.ToJson());
		}
		return j;
	}
	void Human::FromJson(nlohmann::json &j)
	{
		Entity::FromJson(j);
		inventory = {};
		for (auto& i : j["inventory"]){
			ItemData data;
			data.FromJson(i);
			inventory.push_back(data);
		}
	}

}