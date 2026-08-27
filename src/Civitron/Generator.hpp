#pragma once
#include "../PCH.hpp"
#include "Chunk.hpp"
#include "PerlinNoise.hpp"
#include "../json.hpp"
namespace cc
{
	class Generator
	{
	public:
		uint64_t seed;
		Generator();
		void SetSeed(uint64_t seed);
		nlohmann::json ToJson();
		void FromJson(nlohmann::json j);
		Chunk *GenerateChunk(sf::Vector2i position);
	};

}