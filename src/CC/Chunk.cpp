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
				return {w, h};
			}
		}

		// area is prime
		return {1, area};
	}

	void Chunk::WriteData(std::string path)
	{
		// file layout: uint32 byte count, followed by that many bytes of chunk data
		std::ofstream file(path, std::ios::binary);
		std::vector<uint8_t> binaryData = GetByteData();
		uint32_t binarySize = binaryData.size();
		file.write(reinterpret_cast<const char *>(&binarySize), sizeof(binarySize));
		file.write(reinterpret_cast<const char *>(binaryData.data()), binaryData.size());
		file.close();
	}

	void Chunk::ReadData(std::string path)
	{
		std::ifstream file(path, std::ios::binary);

		std::vector<uint8_t> binaryData;
		std::uint32_t binarySize;
		file.read(reinterpret_cast<char *>(&binarySize), sizeof(binarySize));
		binaryData.resize(binarySize);
		file.read(reinterpret_cast<char *>(binaryData.data()), binarySize);

		LoadByteData(binaryData);
		file.close();
	}

	std::vector<uint8_t> Chunk::GetByteData()
	{
		std::vector<uint8_t> bytes;
		bytes.reserve(CHUNK_SIZE * CHUNK_SIZE * 6);

		// tile entity block
		Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
		int n = tileEntities.size();
		s.field("n", n);
		int i = 0;
		for (auto &e : tileEntities)
		{
			std::string index = std::to_string(i);
			s.field(index + " type", e.second->type);
			uint16_t key = e.first;
			s.field(index + " key", key);
			s.field(index + " value", e.second.get());
			i++;
		}
		auto data = s.binary();

		// the block is prefixed with its size so it can be skipped over when reading
		uint32_t size = (uint32_t)data.size();
		uint8_t *sizeBytes = reinterpret_cast<uint8_t *>(&size);
		bytes.insert(bytes.end(), sizeBytes, sizeBytes + sizeof(uint32_t));
		bytes.insert(bytes.end(), data.begin(), data.end());

		// per-tile data
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				uint16_t type = tiles[x][y].type;

				bytes.push_back(static_cast<uint8_t>(type >> 8));
				bytes.push_back(static_cast<uint8_t>(type & 0xFF));
				bytes.push_back(backgroundTiles[x][y].color.r);
				bytes.push_back(backgroundTiles[x][y].color.g);
				bytes.push_back(backgroundTiles[x][y].color.b);
				bytes.push_back((uint8_t)backgroundTiles[x][y].type);
			}
		}

		return bytes;
	}

	void Chunk::LoadByteData(std::vector<uint8_t> &bytes)
	{
		// the data starts with the size of the tile entity block
		uint32_t size;
		std::memcpy(&size, bytes.data(), sizeof(uint32_t));

		// tile entity block
		std::vector<uint8_t> data(
			bytes.begin() + sizeof(uint32_t),
			bytes.begin() + sizeof(uint32_t) + size);

		Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
		int n;
		s.field("n", n);
		for (int i = 0; i < n; i++)
		{
			std::string index = std::to_string(i);
			uint16_t type;
			s.field(index + " type", type);
			auto *e = CreateTileEntityFromType(type);
			uint16_t key;
			s.field(index + " key", key);
			s.field(index + " value", e);
			tileEntities[key] = std::unique_ptr<TileEntity>(e);
		}

		// per-tile data follows the tile entity block
		size_t index = sizeof(uint32_t) + size;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				uint16_t high = bytes[index++];
				uint16_t low = bytes[index++];
				tiles[x][y].type = static_cast<uint16_t>((high << 8) | low);

				uint8_t r = bytes[index++];
				uint8_t g = bytes[index++];
				uint8_t b = bytes[index++];
				backgroundTiles[x][y].color = sf::Color(r, g, b);
				backgroundTiles[x][y].type = (BackgroundTileType)bytes[index++];
			}
		}
	}

	void Chunk::RenderEntities(sf::RenderTarget *target)
	{
		sf::RenderStates states;
		states.texture = &EntityInfo::atlas.texture;

		// reserve room for one quad (6 vertices) per entity; resized to the real count below
		sf::VertexArray arr(sf::PrimitiveType::Triangles, entities.size() * 6);
		int i = 0;
		for (auto &e : entities)
		{
			auto verts = e->GetVerts();
			for (int j = 0; j < verts.size(); j++)
			{
				arr[i] = verts[j];
				i++;
			}
		}
		arr.resize(i);
		target->draw(arr, states);
	}

	void Chunk::AddEntity(Entity *entity)
	{
		entities.push_back(entity);
	}

	void Chunk::RemoveEntity(int index)
	{
		entities.erase(entities.begin() + index);
	}

	void Chunk::RemoveEntity(Entity *entity)
	{
		for (int i = 0; i < entities.size(); i++)
		{
			if (entities[i] == entity)
			{
				RemoveEntity(i);
				return;
			}
		}
	}

	uint16_t Chunk::TileEntityIndex(sf::Vector2i pos)
	{
		return pos.y * CHUNK_SIZE + pos.x;
	}

	void Chunk::SetTile(sf::Vector2i pos, Tile tile, TileEntity *tileEntity)
	{
		const uint16_t entityIndex = TileEntityIndex(pos);
		if (tileEntities.contains(entityIndex))
		{
			RemoveTileEntity(entityIndex);
		}
		tiles[pos.x][pos.y] = tile;

		if (!TileInfo::tileRegistry[tile.type].isTileEntity)
		{
			return;
		}
		if (tileEntity == nullptr)
		{
			tileEntity = CreateTileEntityFromType(tile.type);
		}
		tileEntities[entityIndex] = std::unique_ptr<TileEntity>(tileEntity);
		tileEntity->position = pos;
		tileEntity->chunk = this;
	}

	void Chunk::RemoveTileEntity(uint16_t index)
	{
		// TODO: add some sort of on-delete function, for example for a container holding items that gets destroyed
		tileEntities.erase(index);
	}

	std::pair<Tile *, TileEntity *> Chunk::GetTile(sf::Vector2i pos)
	{
		Tile *t = &tiles[pos.x][pos.y];
		TileEntity *e = nullptr;
		if (TileInfo::tileRegistry[t->type].isTileEntity)
		{
			e = tileEntities[TileEntityIndex(pos)].get();
		}
		return {t, e};
	}
}