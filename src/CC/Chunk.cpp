#include "Chunk.hpp"
#include "TileInfo.hpp"
namespace cc
{
	Chunk::Chunk(sf::Vector2i position)
	{
		this->position = position;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				tiles[x][y].type = 0;
				backgroundTiles[x][y].color = sf::Color::White;
				// int brightness = rand() % 256;
				// backgroundTiles[x][y].color = sf::Color(brightness,brightness,brightness);
			}
		}
	}
	Chunk::Chunk()
	{
		position = {0, 0};
	}

	sf::Vector2i closestSquareDims(int area)
	{
		int side = static_cast<int>(std::sqrt(area));

		for (int w = side; w > 0; --w)
		{
			if (area % w == 0)
			{
				int h = area / w;
				return {w, h}; // width and height
			}
		}

		return {1, area}; // Fallback, area is a prime number
	}

	std::array<uint8_t, CHUNK_NUM_BYTES> Chunk::ToBytes()
	{
		std::array<uint8_t, CHUNK_NUM_BYTES> bytes{};

		size_t index = 0;

		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				uint16_t type = tiles[x][y].type;

				bytes[index++] = static_cast<uint8_t>(type >> 8);
				bytes[index++] = static_cast<uint8_t>(type & 0xFF);
				//TODO: not sure why these need to be bgr instead of rgb, need to investigate
				bytes[index++] = backgroundTiles[x][y].color.b;
				bytes[index++] = backgroundTiles[x][y].color.g;
				bytes[index++] = backgroundTiles[x][y].color.r;
				bytes[index++] = backgroundTiles[x][y].type;
			}
		}

		return bytes;
	}

	void Chunk::FromBytes(
		std::array<uint8_t, CHUNK_NUM_BYTES> &bytes)
	{
		size_t index = 0;

		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				uint16_t type =
					(static_cast<uint16_t>(bytes[index++]) << 8) |
					static_cast<uint16_t>(bytes[index++]);

				tiles[x][y].type = type;

				backgroundTiles[x][y].color = sf::Color(
					bytes[index++],
					bytes[index++],
					bytes[index++]);
				backgroundTiles[x][y].type = (BackgroundTile::BackgroundTileType)bytes[index++];
			}
		}
	}
	void Chunk::RenderEntities(sf::RenderTarget* target)
	{
		sf::RenderStates states;
		states.texture = &EntityInfo::atlas.texture;
		//allows for 1 rectangles per entity
		sf::VertexArray arr(sf::PrimitiveType::Triangles, entities.size() * 6);
		int i = 0;
		for (auto& e : entities)
		{
			auto verts = e->GetVerts();
			for (int j = 0; j < verts.size(); j ++)
			{
				arr[i] = verts[j];
				i ++;
			}
		}
		arr.resize(i);
		target->draw(arr,states);
	}
	void Chunk::AddEntity(Entity* entity)
	{
		entities.push_back(entity);
	}
	void Chunk::RemoveEntity(int index)
	{
		entities.erase(entities.begin() + index);
	}
	void Chunk::RemoveEntity(Entity* entity)
	{
		for (int i = 0; i < entities.size(); i ++)
		{
			if (entities[i] == entity)
			{
				RemoveEntity(i);
				return;
			}
		}
	}
}