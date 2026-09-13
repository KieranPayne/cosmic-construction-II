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
				return {w, h}; // width and height
			}
		}

		return {1, area}; // Fallback, area is a prime number
	}
	void Chunk::WriteData(std::string path)
	{
		// DATA IS STORED AS:
		// STR_LEN STR BIN_LEN BIN
		std::ofstream file(path, std::ios::binary);
		// writing string
		std::string stringData = GetStringData();
		std::cout << stringData << std::endl;
		uint32_t strSize = stringData.size();
		file.write(reinterpret_cast<const char *>(&strSize), sizeof(strSize));
		file.write(stringData.data(), stringData.size());
		// writing binary
		std::vector<uint8_t> binaryData = GetByteData();
		uint32_t binarySize = binaryData.size();
		file.write(reinterpret_cast<const char *>(&binarySize), sizeof(binarySize));
		file.write(reinterpret_cast<const char *>(binaryData.data()), binaryData.size());
		file.close();
	}
	void Chunk::ReadData(std::string path)
	{
		std::ifstream file(path, std::ios::binary);
		uint32_t textSize;
		file.read(reinterpret_cast<char *>(&textSize), sizeof(textSize));
		std::string text;
		text.resize(textSize);
		file.read(text.data(), textSize);

		std::vector<uint8_t> binaryData;
		std::uint32_t binarySize;
		file.read(reinterpret_cast<char *>(&binarySize), sizeof(binarySize));
		binaryData.resize(binarySize);
		file.read(reinterpret_cast<char *>(binaryData.data()), binarySize);

		LoadStringData(text);
		LoadByteData(binaryData);
		file.close();
	}
	std::vector<uint8_t> Chunk::GetByteData()
	{
		std::vector<uint8_t> bytes;
		bytes.reserve(CHUNK_SIZE * CHUNK_SIZE * 6);

		size_t index = 0;

		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				uint16_t type = tiles[x][y].type;

				bytes.push_back(static_cast<uint8_t>(type >> 8));
				bytes.push_back(static_cast<uint8_t>(type & 0xFF));
				// TODO: not sure why these need to be bgr instead of rgb, need to investigate
				bytes.push_back(backgroundTiles[x][y].color.r);
				bytes.push_back(backgroundTiles[x][y].color.g);
				bytes.push_back(backgroundTiles[x][y].color.b);
				bytes.push_back((uint8_t) backgroundTiles[x][y].type);
			}
		}

		return bytes;
	}

	void Chunk::LoadByteData(std::vector<uint8_t> &bytes)
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
				uint8_t r = bytes[index++];
				uint8_t g = bytes[index++];
				uint8_t b = bytes[index++];
				backgroundTiles[x][y].color = sf::Color(r, g, b);
				backgroundTiles[x][y].type = (BackgroundTileType)bytes[index++];
			}
		}
	}

	std::string Chunk::GetStringData()
	{
		nlohmann::json arr;
		for (auto &e : tileEntities)
		{
			nlohmann::json j;
			j["key"] = e.first;
			j["value"] = e.second->ToJson();
			arr.push_back(j);
		}
		return arr.dump(2);
	}

	void Chunk::LoadStringData(std::string &data)
	{
		nlohmann::json arr = nlohmann::json::parse(data);
		for (auto &j : arr)
		{
			uint16_t key = j["key"];
			uint16_t type = j["value"]["type"];
			TileEntity *entity = CreateTileEntityFromType(type);
			tileEntities[key] = std::unique_ptr<TileEntity>(entity);
		}
	}

	void Chunk::RenderEntities(sf::RenderTarget *target)
	{
		sf::RenderStates states;
		states.texture = &EntityInfo::atlas.texture;
		// allows for 1 rectangles per entity
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
		if (tileEntities.contains(TileEntityIndex(pos)))
		{
			RemoveTileEntity(TileEntityIndex(pos));
		}
		tiles[pos.x][pos.y] = tile;
		if (TileInfo::tileRegistry[tile.type].isTileEntity)
		{
			int index = TileEntityIndex(pos);
			if (tileEntity != nullptr)
			{
				tileEntities[index] = std::unique_ptr<TileEntity>(tileEntity);
			}
			else
			{
				tileEntities[index] = std::unique_ptr<TileEntity>(CreateTileEntityFromType(tile.type));
			}
			tileEntities[index]->position = pos;
			tileEntities[index]->chunk = this;
			
		}
	}
	void Chunk::RemoveTileEntity(uint16_t index)
	{
		// TODO: add some sort of ondelete function. this would be used if a container holding items gets destroyed for example
		tileEntities.erase(index);
	}
	std::pair<Tile*, TileEntity*> Chunk::GetTile(sf::Vector2i pos)
	{
		Tile* t = &tiles[pos.x][pos.y];
		TileEntity* e = nullptr;
		if (TileInfo::tileRegistry[t->type].isTileEntity)
		{
			e = tileEntities[TileEntityIndex(pos)].get();
		}
		return {t,e};
	}
}