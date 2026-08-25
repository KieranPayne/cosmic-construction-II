#include "Entity.hpp"
#include "Chunk.hpp"
#include "EntityInfo.hpp"
#include "utils.hpp"
#include "Human.hpp"
#include "Planet.hpp"
#include "TileInfo.hpp"
#include "Item.hpp"
namespace Civitron
{
	Entity::Entity()
	{
		position = {0.f,0.f};
	}
	std::vector<sf::Vertex> Entity::GetVertices()
	{
		// sf::VertexArray arr(sf::PrimitiveType::Triangles, 6);
		std::vector<sf::Vertex> arr;
		for (int i = 0; i < 6; i ++){
			arr.push_back(sf::Vertex());
		}
		float x = position.x * TILE_SIZE;
		float y = position.y * TILE_SIZE; // mapping z → y for 2D plane

		// First triangle
		arr[0].position = sf::Vector2f(x, y);
		arr[1].position = sf::Vector2f(x + TILE_SIZE * size.x, y);
		arr[2].position = sf::Vector2f(x + TILE_SIZE * size.x, y + TILE_SIZE * size.y);

		// Second triangle
		arr[3].position = sf::Vector2f(x, y);
		arr[4].position = sf::Vector2f(x + TILE_SIZE * size.x, y + TILE_SIZE * size.y);
		arr[5].position = sf::Vector2f(x, y + TILE_SIZE * size.y);
		{
			sf::Vector2f pos = GetTexCoords();
			float x = pos.x;
			float y = pos.y;
			arr[0].texCoords = sf::Vector2f(x, y);
			arr[1].texCoords = sf::Vector2f(x + TILE_SIZE * size.x, y);
			arr[2].texCoords = sf::Vector2f(x + TILE_SIZE * size.x, y + TILE_SIZE * size.y);

			// Second triangle
			arr[3].texCoords = sf::Vector2f(x, y);
			arr[4].texCoords = sf::Vector2f(x + TILE_SIZE * size.x, y + TILE_SIZE * size.y);
			arr[5].texCoords = sf::Vector2f(x, y + TILE_SIZE * size.y);
		}
		return arr;
	}
	sf::Vector2f Entity::GetTexCoords()
	{
		return JsonAsVector(EntityInfo::texturesJson["Entity"]);
	}

	nlohmann::json Entity::ToJson()
	{
		nlohmann::json j;
		j["classType"] = myClass;
		j["pos"] = {position.x, position.y};
		return j;
	}
	void Entity::FromJson(nlohmann::json &j)
	{
		position.x = j["pos"][0];
		position.y = j["pos"][1];
	}
	// static function that loads object based on class
	Entity *Entity::LoadFromJson(nlohmann::json &j)
	{
		Entity *e;
		if (j["classType"] == NONE)
		{
			e = new Entity();
		}
		else if (j["classType"] == HUMAN)
		{
			e = new Human();
		}
		else if (j["classType"] == ITEM)
		{
			e = new Item();
		}
		e->FromJson(j);
		return e;
	}
	void Entity::Tick(Planet *planet)
	{
	}
	void Entity::MoveTo(sf::Vector2f newPos){
		sf::Vector2i chunkPos = TileToChunkPos({(int)floor(position.x),(int)floor(position.y)});
		sf::Vector2i newChunkPos = TileToChunkPos({(int)floor(newPos.x),(int)floor(newPos.y)});
		if (chunkPos != newChunkPos){
			planet->chunks[chunkPos]->RemoveEntity(this);
			planet->chunks[newChunkPos]->entities.push_back(this);
		}
		position = newPos;

	}
}
