#pragma once
#include "../PCH.hpp"
#include "Chunk.hpp"
#include "PerlinNoise.hpp"

namespace cc
{
	class Generator
	{
	public:
		Generator();
		Chunk *GenerateChunk(sf::Vector2i position);
	};

}