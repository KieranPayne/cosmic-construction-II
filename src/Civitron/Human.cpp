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
		position = {0, 0, 0};
		myClass = HUMAN;
		path = {};
	}
	sf::Vector2f Human::GetTexCoords(int height)
	{
		return JsonAsVector(EntityInfo::texturesJson["Human"]);
	}
	void Human::Tick(Planet *planet)
	{
		if (planet->GetTileAt(position - sf::Vector3i(0, 1, 0))->type == GetTileID("Air"))
		{
			if (justJumped)
			{
				justJumped = false;
			}
			else
			{
				position -= sf::Vector3i(0, 1, 0);
				// move chunk position if moved down to chunk below
				if (((position.y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE == CHUNK_SIZE - 1)
				{
					auto chunkPos = TileToChunkPos(position);
					planet->chunks[chunkPos + sf::Vector3i(0, 1, 0)]->RemoveEntity(this);
					planet->chunks[chunkPos]->entities.push_back(this);
				}
				return;
			}
		}
		if (path.size() == 1)
		{
			if (planet->GetTileAt(path[0])->type == GetTileID("Log"))
			{
				std::deque<sf::Vector3i> logs;
				logs.push_back(path[0]);
				while (logs.size() > 0)
				{
					sf::Vector3i pos = logs.front();
					logs.pop_front();
					if (planet->GetTileAt(pos)->type == GetTileID("Log"))
					{
						Item *item = new Item();
						item->position = pos;
						item->data = {0, 1};
						planet->AddEntity(item);
				}

					planet->SetTileAt(pos, Tile(GetTileID("Air")));
					for (int x = -1; x < 2; x++)
					{
						for (int y = -1; y < 2; y++)
						{
							for (int z = -1; z < 2; z++)
							{
								int n = (x != 0) + (y != 0) + (z != 0);
								if (n > 1)
								{
									continue;
								}
								sf::Vector3i p = pos + sf::Vector3i(x, y, z);
								if (std::find(logs.begin(), logs.end(), p) != logs.end())
								{
									continue;
								}
								sf::Vector3i chunkPos = TileToChunkPos(p);
								auto type = planet->GetTileAt(p)->type;
								if (type == GetTileID("Log") || type == GetTileID("Leaves"))
								{
									logs.push_back(p);
								}
							}
						}
					}
				}
				path = {};
				return;
			}
			if (path[0] != position)
			{
				path = {};
			}
			return;
		}
		if (path.size() > 0)
		{
			if (path[0].y != position.y)
			{
				if (path[0].y > position.y)
				{
					MoveTo(position + sf::Vector3i(0, 1, 0));
					justJumped = true;
				}
				else
				{
					MoveTo(sf::Vector3i(path[0].x, position.y, path[0].z));
				}
				return;
			}
			MoveTo(path[0]);
			path.erase(path.begin());
			return;
		}
		auto result = Pathfinding::SearchForTile(position, {GetTileID("Log")}, planet, {true, 60});
		if (!result.second)
		{
			path = {position};
			return;
		}
		path = Pathfinding::PathFind(position, result.first, planet, {true, 60});
	}

	nlohmann::json Human::ToJson()
	{
		nlohmann::json j = Entity::ToJson();
		j["justJumped"] = justJumped;
		j["path"] = {};
		for (int i = 0; i < path.size(); i++)
		{
			j["path"].push_back({path[i].x, path[i].y, path[i].z});
		}
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
		justJumped = j["justJumped"];
		for (int i = 0; i < j["path"].size(); i++)
		{
			path.push_back(sf::Vector3i(j["path"][i][0], j["path"][i][1], j["path"][i][2]));
		}
	}

}