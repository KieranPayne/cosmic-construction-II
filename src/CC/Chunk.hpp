#pragma once
#include "../PCH.hpp"
#include "Tile.hpp"
#include "Entity.hpp"
namespace cc
{
#define CHUNK_SIZE 32
constexpr int CHUNK_NUM_BYTES = CHUNK_SIZE * CHUNK_SIZE * (2 + 4); 
	class Chunk
	{
	public:
		Tile tiles[CHUNK_SIZE][CHUNK_SIZE] = {};
		BackgroundTile backgroundTiles[CHUNK_SIZE][CHUNK_SIZE] = {};
		sf::Vector2i position;
		std::vector<Entity*> entities;
		Chunk(sf::Vector2i position);
		Chunk();
		std::array<uint8_t,CHUNK_NUM_BYTES> ToBytes();
		void FromBytes(std::array<uint8_t,CHUNK_NUM_BYTES>&);
		void RenderEntities(sf::RenderTarget* target);
		void AddEntity(Entity* entity);
		void RemoveEntity(Entity* entity);
		void RemoveEntity(int index);
	};

	struct ChunkHash
	{
		std::size_t operator()(const sf::Vector2i& pos) const
    {
        std::uint64_t x = static_cast<std::uint32_t>(pos.x);
        std::uint64_t y = static_cast<std::uint32_t>(pos.y);

        std::uint64_t h = x * 0x9E3779B185EBCA87ULL;
        h ^= y + 0x9E3779B185EBCA87ULL + (h << 6) + (h >> 2);

        return static_cast<std::size_t>(h);
    }
	};

}