#pragma once
#include "../PCH.hpp"
#include "Tile.hpp"
#include "Entity.hpp"
namespace Civitron
{
#define CHUNK_SIZE 32
constexpr int CHUNK_NUM_BYTES = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 2; 
	class Chunk
	{
	public:
		Tile tiles[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] = {};
		sf::Vector3i position;
		std::vector<Entity *> entities;
		Chunk(sf::Vector3i position);
		Chunk();
		void RemoveEntity(Entity *entity);
		std::array<uint8_t,CHUNK_NUM_BYTES> ToBytes();
		void FromBytes(std::array<uint8_t,CHUNK_NUM_BYTES>);
	};

	struct ChunkHash
	{
		std::size_t operator()(const sf::Vector3i &v) const
		{
			int x0 = 0x123456;
			int m = 67;
			int x1 = (std::hash<int>()(v.x) xor x0) * m;
			int x2 = (std::hash<int>()(v.y) xor x1) * m;
			int x3 = (std::hash<int>()(v.z) xor x2) * m;
			return x3;
			// Combine the hashes (boost-like)
		}
	};

}