#include "Planet.hpp"
#include "../Timer.hpp"
#include "PerlinNoise.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "TileInfo.hpp"
#include "../imgui/imgui.h"
#include "../Main.hpp"
#include <queue>
#include "Human.hpp"
#include "JsonEditor.hpp"
#include "Item.hpp"
#include "Serializer.hpp"
#include "CSMessage.hpp"
#include "Client.hpp"
namespace cc
{
	Planet::Planet()
	{
		index = -1;
		// chunks.clear();
		// camera = Camera();
		currFps = 0;
		generator.planet = this;
	}
	void Planet::Update(double dt)
	{
		if (isServerPlanet)
		{
			for (auto &e : entities)
			{
				if (!e->isInChunk)
				{
					if (!chunks.contains(e->chunkPos))
					{
						GenerateChunk(e->chunkPos);
					}
					chunks[e->chunkPos]->AddEntity(e.get());
					e->isInChunk = true;
				}
			}
		}
		if (tileSetRequests.size() > 0)
		{
			for (int i = 0; i < tileSetRequests.size(); i ++)
			{
				auto& t = tileSetRequests[i];
				sf::Vector2i chunkPos = t.first;
				if (chunks.contains(chunkPos))
				{
					SetTileAt(t.first,t.second.first,t.second.second);
					tileSetRequests.erase(tileSetRequests.begin() + i);
					i --;
				}
			}
		}
	}
	void Planet::VisibleUpdate(sf::RenderTarget *target, InputState &inputState, double dt)
	{
		camera.Update(dt, inputState);
		DrawInfoGUI(dt);
		DrawToolGUI(inputState);
		jsonEditor.Draw(this);
	}
	std::vector<sf::Vector2i> Planet::GetChunksToRequest(sf::RenderTarget *target)
	{
		sf::FloatRect view = camera.toFloatRect(target);
		constexpr int chunkSizePixels = CHUNK_SIZE * TILE_SIZE;
		sf::Vector2i topLeft = {(int)floor(view.position.x / chunkSizePixels), (int)floor(view.position.y / chunkSizePixels)};
		sf::Vector2i bottomRight = {(int)floor((view.position.x + view.size.x) / chunkSizePixels), (int)floor((view.position.y + view.size.y) / chunkSizePixels)};
		topLeft -= {1, 1};
		bottomRight += {1, 1};
		std::vector<sf::Vector2i> positions = {};
		for (int x = topLeft.x; x <= bottomRight.x; x++)
		{
			for (int z = topLeft.y; z <= bottomRight.y; z++)
			{

				if (!chunks.contains({x, z}))
				{
					positions.push_back({x, z});
				}
			}
		}
		for (auto &e : entities)
		{
			if (!e->isInChunk)
			{
				if (chunks.contains(e->chunkPos))
				{
					chunks[e->chunkPos]->AddEntity(e.get());
					e->isInChunk = true;
				}
				else
				{
					if (std::find(positions.begin(), positions.end(), e->chunkPos) == positions.end())
					{
						positions.push_back(e->chunkPos);
					}
				}
			}
		}
		for (int i = 0; i < tileSetRequests.size();i ++)
		{
			sf::Vector2i chunkPos = TileToChunkPos(tileSetRequests[i].first);
			if (!chunks.contains(chunkPos))
			{
				positions.push_back(chunkPos);
			}
		}
		for (int i = 0; i < positions.size(); i ++)
		{
			if (chunksRequested.contains(positions[i]))
			{
				positions.erase(positions.begin() + i);
				i --;
			}else
			{
				chunksRequested.emplace(positions[i]);
			}
		}
		return positions;
	}
	void Planet::Render(sf::RenderTarget *target)
	{
		sf::RenderStates states;
		states.texture = &TileInfo::atlas.texture;
		sf::FloatRect view = camera.toFloatRect(target);
		constexpr int chunkSizePixels = CHUNK_SIZE * TILE_SIZE;
		sf::Vector2i topLeft = {(int)floor(view.position.x / chunkSizePixels), (int)floor(view.position.y / chunkSizePixels)};
		sf::Vector2i bottomRight = {(int)floor((view.position.x + view.size.x) / chunkSizePixels), (int)floor((view.position.y + view.size.y) / chunkSizePixels)};
		std::vector<sf::Vertex> vertices;
		for (int x = topLeft.x; x <= bottomRight.x; x++)
		{
			for (int y = topLeft.y; y <= bottomRight.y; y++)
			{
				if (!chunks.contains({x, y}))
				{
					continue;
				}
				if (!tileVertices.contains({x, y}))
				{
					GetTileVertices({x, y});
				}
				target->draw(tileVertices[{x, y}], states);
			}
		}
		// draw entities
		for (int x = topLeft.x - 1; x <= bottomRight.x + 1; x++)
		{
			for (int y = topLeft.y - 1; y <= bottomRight.y + 1; y++)
			{
				if (!chunks.contains({x, y}))
				{
					continue;
				}
				chunks[{x, y}]->RenderEntities(target);
			}
		}
	}
	void Planet::Save()
	{
		std::string path = SaveManager::savePath + "/planets";
		if (!SaveManager::DirExists(path))
		{
			SaveManager::CreateDirectory(path);
		}
		path += "/" + std::to_string(index);
		if (!SaveManager::DirExists(path))
		{
			SaveManager::CreateDirectory(path);
		}
		if (!SaveManager::DirExists(path + "/chunks"))
		{
			SaveManager::CreateDirectory(path + "/chunks");
		}
		// save chunks
		for (auto &c : chunks)
		{
			std::string chunkPath = path + "/chunks/";
			chunkPath += std::to_string(c.second->position.x) + " " + std::to_string(c.second->position.y);
			c.second->WriteData(chunkPath + ".txt");
		}
		// save entities
		Serializer entityData(Serializer::Mode::WRITE, SaveManager::saveFormat);
		int n = entities.size();
		entityData.field("n", n);
		// nlohmann::json entityData;
		for (int i = 0; i < n; i++)
		{
			uint16_t type = (uint16_t)entities[i]->type;
			entityData.field(std::to_string(i) + " type", type);
			entityData.field(std::to_string(i), entities[i].get());
		}
		SaveManager::WriteSerializerToFile(entityData, path + "/entities");
		// SaveManager::WriteData(path + "/entities.json", entityData.dump(2));
		// misc variables get saved in planet json
		Serializer s(Serializer::Mode::WRITE, SaveManager::saveFormat);
		Serialize(s);
		SaveManager::WriteSerializerToFile(s, path + "/planet");
		// SaveManager::WriteData(path + "/planet.json", ToJson().dump(2));
		generator.Save(path);
	}
	void Planet::Load()
	{
		std::string path = SaveManager::savePath + "/planets";
		if (!SaveManager::DirExists(path))
		{
			return;
		}
		path += "/" + std::to_string(index);
		if (!SaveManager::DirExists(path))
		{
			return;
		}
		// load chunks
		std::string chunkPath = path + "/chunks";
		auto files = SaveManager::ListFiles(chunkPath);
		for (auto &f : files)
		{
			auto coords = Split(f.substr(chunkPath.size() + 1), ' ');
			// remove file extension
			coords[1] = coords[1].substr(0, coords[1].size() - 4);
			sf::Vector2i vec{std::stoi(coords[0]), std::stoi(coords[1])};
			chunks[vec] = std::make_unique<Chunk>(vec);
			chunks[vec]->ReadData(f);
		}
		// load entities
		Serializer entityData = SaveManager::LoadSerializerFromFile(path + "/entities");
		int n;
		entityData.field("n", n);
		// nlohmann::json entityData = nlohmann::json::parse(SaveManager::ReadData(path + "/entities.json"));
		for (int i = 0; i < n; i++)
		{
			uint16_t type;
			entityData.field(std::to_string(i) + " type", type);
			Entity *e = CreateEntityFromType((Entity::EntityType)type);
			entityData.field(std::to_string(i), e);
			AddEntity(e, true);
		}
		// load misc data
		Serializer s = SaveManager::LoadSerializerFromFile(path + "/planet");
		Serialize(s);
		// FromJson(nlohmann::json::parse(SaveManager::ReadData(path + "/planet.json")));
		generator.Load(path);
	}
	void Planet::AddEntity(Entity *entity, bool sentByServer)
	{
		sf::Vector2i chunkPos = TileToChunkPos(entity->position);
		entity->chunkPos = chunkPos;
		entities.push_back(std::unique_ptr<Entity>(entity));
		// TODO: deal with case where dont have chunk yet
		if (!chunks.contains(chunkPos))
		{
			return;
			// chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		if (!sentByServer)
		{
			sf::Packet p;
			p << (uint16_t)CSMessageType::REQUEST_ADD_ENTITY;
			AppendEntityToPacket(p, entity);
			client->SendPacket(p);
		}
		entity->isInChunk = true;
		chunks[chunkPos]->AddEntity(entity);
	}
	void Planet::DrawInfoGUI(double dt)
	{
		if (macro.active)
		{
			return;
		}
		static int currentView = -1;
		ImGui::SetNextWindowPos(ImVec2(321, 4), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(362, 183), ImGuiCond_Once);
		ImGui::Begin("World Info");
		double fps = 1.0 / (dt + 0.0000000001);
		currFps += (fps - currFps) * dt * 3;
		ImGui::Text(("FPS: " + std::to_string(currFps)).c_str());
		// ImGui::SliderInt("Tracking Entity", &trackingEntity, -1, entities.size() - 1);

		// bgTile += "Colour: " + std::to_string()
		const char *currentLabel = "None";
		switch (currentView)
		{
		case 0:
			currentLabel = "Camera";
			break;
		case 1:
			currentLabel = "Entities";
			break;
		}
		if (ImGui::BeginCombo("##", currentLabel))
		{
			if (ImGui::Selectable("None", currentView == -1))
				currentView = -1;
			if (ImGui::Selectable("Camera", currentView == 0))
				currentView = 0;
			if (ImGui::Selectable("Entities", currentView == 1))
				currentView = 1;
			ImGui::EndCombo();
		}
		ImGui::Separator();
		if (currentView == 0)
		{
			Serializer s(Serializer::Mode::WRITE, Serializer::Format::JSON);
			camera.Serialize(s);
			ImGui::Text(s.json().dump(2).c_str());
		}
		else if (currentView == 1)
		{
			std::string result = "";
			for (auto &e : entities)
			{
				Serializer s(Serializer::Mode::WRITE, Serializer::Format::JSON);
				e->Serialize(s);
				result += s.json().dump(2) + "\n";
			}
			ImGui::Text(result.c_str());
		}
		ImGui::End();
	}
	// TODO: make this return a pair that also returns the tile entity if there is one here
	std::pair<Tile *, TileEntity *> Planet::GetTileAt(sf::Vector2i position)
	{
		sf::Vector2i chunkPos = TileToChunkPos(position);
		if (!chunks.contains(chunkPos))
		{
			chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		sf::Vector2i subChunkPos = position - chunkPos * CHUNK_SIZE;
		// return &chunks[chunkPos]->tiles[subChunkPos.x][subChunkPos.y];
		return chunks[chunkPos]->GetTile(subChunkPos);
	}
	void Planet::SetTileAt(sf::Vector2i position, Tile tile, TileEntity *tileEntity, bool sentByServer)
	{
		sf::Vector2i chunkPos = TileToChunkPos(position);
		if (!chunks.contains(chunkPos))
		{
			if (isServerPlanet)
			{
				GenerateChunk(chunkPos);
			}else
			{
				tileSetRequests.push_back({position,{tile,tileEntity}});
				return;
			}
		}
		auto current = GetTileAt(position);
		if (current.first->type == tile.type)
		{
			if (tileEntity != nullptr)
			{
				if (current.second != nullptr)
				{
					Serializer s1(Serializer::Mode::WRITE, Serializer::Format::BINARY);
					tileEntity->Serialize(s1);
					Serializer s2(Serializer::Mode::WRITE, Serializer::Format::BINARY);
					current.second->Serialize(s2);
					if (s1.binary() == s2.binary())
					{
						return;
					}
				}
			}
			else if (tileEntity == nullptr && current.second == nullptr)
			{
				return;
			}
		}
		sf::Vector2i subChunkPos = position - chunkPos * CHUNK_SIZE;
		if (client != nullptr && !sentByServer)
		{
			sf::Packet p;
			p << (uint16_t)CSMessageType::REQUEST_SET_TILE;
			p << position.x << position.y;
			p << tile.type;
			p << (bool)(tileEntity != nullptr);
			if (tileEntity != nullptr)
			{
				Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);

				tileEntity->Serialize(s);
				auto data = s.binary();
				p << (uint64_t)data.size();
				p.append(data.data(), data.size());
			}
			client->SendPacket(p);
		}
		chunks[chunkPos]->SetTile(subChunkPos, tile, tileEntity);
		// chunks[chunkPos]->tiles[subChunkPos.x][subChunkPos.y] = tile;
		if (tileVertices.contains(chunkPos))
		{
			// erase the vertices since they're no longer accurate
			// note: erase instead of rebuilding since these may not necessarily be visible, so this would be wasted effort.
			tileVertices.erase(chunkPos);
		}
	}
	void Planet::Tick()
	{
		std::vector<Entity *> entitiesToMove;
		std::vector<sf::Vector2i> oldPoses;
		std::vector<sf::Vector2i> newPoses;

		for (auto &e : entities)
		{
			e->Tick();
			sf::Vector2i newChunkPos = TileToChunkPos(e->position);
			if (newChunkPos != e->chunkPos)
			{
				entitiesToMove.push_back(e.get());
				newPoses.push_back(newChunkPos);
			}
		}
		for (int i = 0; i < entitiesToMove.size(); i++)
		{
			MoveEntity(entitiesToMove[i], newPoses[i]);
		}
	}
	void Planet::MoveEntity(Entity *entity, sf::Vector2i newPos)
	{
		chunks[entity->chunkPos]->RemoveEntity(entity);
		if (!chunks.contains(newPos))
		{
			chunks[newPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(newPos));
		}
		chunks[newPos]->AddEntity(entity);
		entity->chunkPos = newPos;
	}
	std::pair<std::vector<sf::Vertex>, bool> Planet::GetVertices(sf::Vector2i tilePosition)
	{
		sf::Vector2i chunkPos = TileToChunkPos(tilePosition);
		Chunk *chunk = chunks[chunkPos].get();
		sf::Vector2i subChunkPos = tilePosition - chunkPos * CHUNK_SIZE;
		float alpha = 255.f;
		static float falloff = 0.85f;
		Tile *t = &chunk->tiles[subChunkPos.x][subChunkPos.y];
		if (t->type != GetTileID("Air"))
		{
			std::vector<sf::Vertex> vertices = {};
			static sf::Vector2f offsets[6] = {
				{0, 0},
				{TILE_SIZE, 0},
				{TILE_SIZE, TILE_SIZE},
				{0, 0},
				{TILE_SIZE, TILE_SIZE},
				{0, TILE_SIZE}};
			sf::Vector2f worldPos = {(float)(tilePosition.x * TILE_SIZE), (float)(tilePosition.y * TILE_SIZE)};
			sf::Vector2f texPos = (sf::Vector2f)TileInfo::tileRegistry[t->type].positions[0];
			for (int i = 0; i < 6; i++)
			{
				vertices.push_back({worldPos + offsets[i], sf::Color(alpha, alpha, alpha, 255), texPos + offsets[i]});
			}
			return {vertices, true};
		}
		return {{}, false};
	}
	void Planet::DrawToolGUI(InputState &inputState)
	{
		ImGui::SetNextWindowPos(ImVec2(4, 316), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(231, 400), ImGuiCond_Once);
		ImGui::Begin("Tool Menu");
		sf::Vector2f worldPos = camera.ToWorldPos(inputState.mousePosition, window.get());
		sf::Vector2i worldTilePos(floor((float)worldPos.x / TILE_SIZE), floor((float)worldPos.y / TILE_SIZE));
		sf::Vector2i worldChunkPos = TileToChunkPos(worldTilePos);
		if (!chunks.contains(worldChunkPos))
		{
			ImGui::End();
			return;
		}
		ImGui::Text("Hovering over:");
		std::string bgTile = "Background Tile:\n";

		sf::Vector2i subChunkPos = worldTilePos - worldChunkPos * CHUNK_SIZE;
		BackgroundTile *bgT = &chunks[worldChunkPos]->backgroundTiles[subChunkPos.x][subChunkPos.y];
		bgTile += "Colour: " + std::to_string(bgT->color.r) + " " + std::to_string(bgT->color.g) + " " + std::to_string(bgT->color.b) + "\n";
		bgTile += "Type: " + std::to_string((uint8_t)bgT->type);
		ImGui::Text(bgTile.c_str());
		std::string tile = "Tile:\n";
		auto t = GetTileAt(worldTilePos);
		tile += "Type: " + TileInfo::tileRegistry[t.first->type].name;
		if (t.second != nullptr)
		{
			tile += "\nTile Entity data:\n";
			Serializer s(Serializer::Mode::WRITE, Serializer::Format::JSON);
			s.field("Tile Entity", t.second);
			tile += s.json().dump(2);
		}
		ImGui::Text(tile.c_str());
		static int currentView = 0;
		std::vector<std::string> names = {"Add Tile", "Insert image"};
		const char *currentLabel = "None";
		currentLabel = names[currentView].c_str();
		if (ImGui::BeginCombo("##", currentLabel))
		{
			for (int i = 0; i < names.size(); i++)
			{
				if (ImGui::Selectable(names[i].c_str(), currentView == i))
					currentView = i;
			}
			ImGui::EndCombo();
		}
		ImGui::Separator();
		if (currentView == 0)
		{
			std::vector<std::string> tileNames = {};
			for (int i = 0; i < TileInfo::tileRegistry.size(); i++)
			{
				tileNames.push_back(TileInfo::tileRegistry[i].name);
			}
			static int currentTile = 0;
			const char *currentLabel = "None";
			currentLabel = tileNames[currentTile].c_str();
			if (ImGui::BeginCombo("## test", currentLabel))
			{
				for (int i = 0; i < tileNames.size(); i++)
				{
					if (ImGui::Selectable(tileNames[i].c_str(), currentTile == i))
						currentTile = i;
				}
				ImGui::EndCombo();
			}
			if (inputState.Down(sf::Mouse::Button::Right))
			{
				sf::Vector2f worldPos = camera.ToWorldPos(inputState.mousePosition, window.get());
				sf::Vector2i worldTilePos(floor((float)worldPos.x / TILE_SIZE), floor((float)worldPos.y / TILE_SIZE));
				SetTileAt(sf::Vector2i(worldTilePos.x, worldTilePos.y), Tile(currentTile));
				// sf::Packet p;
				// p << (uint16_t)CSMessageType::REQUEST_SET_TILE;
			}
		}
		else if (currentView == 1)
		{
			// Persistent state (members of your class, or statics)
			static char pathBuf[256] = "assets/image.png";
			static int  placeXY[2]   = {0, 0};
			static int  width        = 64;
			static std::string status;

			if (ImGui::Begin("Image to Tiles"))
			{
				ImGui::InputText("Image path", pathBuf, sizeof(pathBuf));
				ImGui::InputInt2("Place position (x, y)", placeXY);
				ImGui::InputInt("Width (tiles)", &width);
				width = std::clamp(width, 1, 1024);   // sanity limit, adjust to taste

				if (ImGui::Button("Generate"))
				{
					std::string path = pathBuf;       // function takes a non-const std::string&
					MakeImageFromTiles(path, sf::Vector2i{placeXY[0], placeXY[1]}, width);
					status = "Placed " + path;
				}

				if (!status.empty())
					ImGui::TextUnformatted(status.c_str());
			}
			ImGui::End();
		}
		ImGui::End();
	}
	void Planet::Serialize(Serializer &s)
	{
		s.field("camera", camera);
		s.field("seed", seed);
		// s.field("generator",generator);
	}
	// nlohmann::json Planet::ToJson()
	// {
	// 	nlohmann::json j;
	// 	j["camera"] = camera.ToJson();
	// 	j["seed"] = seed;
	// 	// j["generator"] = generator.ToJson();
	// 	return j;
	// }
	// void Planet::FromJson(nlohmann::json j)
	// {
	// 	camera.FromJson(j["camera"]);
	// 	// generator.FromJson(j["generator"]);
	// }
	void Planet::GetTileVertices(sf::Vector2i chunkPos)
	{
		sf::VertexArray arr(sf::PrimitiveType::Triangles, CHUNK_SIZE * CHUNK_SIZE * 6 * 2);
		// get background tile vertices
		static sf::Vector2f offsets[6] = {
			{0, 0},
			{TILE_SIZE, 0},
			{TILE_SIZE, TILE_SIZE},
			{0, 0},
			{TILE_SIZE, TILE_SIZE},
			{0, TILE_SIZE}};
		int i = 0;
		Chunk *c = chunks[chunkPos].get();
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				for (int j = 0; j < 6; j++)
				{
					sf::Vertex v;
					v.position = (sf::Vector2f)(chunkPos * CHUNK_SIZE * TILE_SIZE) + sf::Vector2f{x * TILE_SIZE, y * TILE_SIZE} + offsets[j];
					v.color = c->backgroundTiles[x][y].color;

					arr[i] = v;
					i++;
				}
			}
		}
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				Tile *t = &c->tiles[x][y];

				// air tile; forgive the magic number pls
				if (t->type == 0)
				{
					continue;
				}
				if (TileInfo::tileRegistry[t->type].isTileEntity)
				{
					c->tileEntities[c->TileEntityIndex({x, y})]->GetVertices(arr, i);
					continue;
				}
				sf::Vector2f texPos = (sf::Vector2f)TileInfo::tileRegistry[t->type].positions[0];
				for (int j = 0; j < 6; j++)
				{
					sf::Vertex v;
					v.position = (sf::Vector2f)(chunkPos * CHUNK_SIZE * TILE_SIZE) + sf::Vector2f{x * TILE_SIZE, y * TILE_SIZE} + offsets[j];
					v.texCoords = texPos + offsets[j];
					arr[i] = v;
					i++;
				}
			}
		}
		arr.resize(i);
		tileVertices[chunkPos] = arr;
	}
	void Planet::SetSeed(uint64_t seed)
	{
		this->seed = seed;
		generator.SetSeed(seed);
	}
	void Planet::GenerateChunk(sf::Vector2i position)
	{
		chunks[position] = std::unique_ptr<Chunk>(generator.GenerateChunk(position));
	}
	void Planet::ReplaceEntity(int index, Entity *e)
	{
		sf::Vector2i chunkPos = TileToChunkPos(entities[index]->position);
		if (chunks.contains(chunkPos))
		{
			chunks[chunkPos]->RemoveEntity(entities[index].get());
		}
		entities[index].reset(e);
		sf::Vector2i newPos = TileToChunkPos(entities[index]->position);
		if (chunks.contains(newPos))
		{
			chunks[newPos]->AddEntity(e);
			e->isInChunk = true;
		}
		else
		{
			e->isInChunk = false;
		}
		e->UpdateChunkPos();
	}
	void Planet::MakeImageFromTiles(std::string &imagePath, sf::Vector2i placePos, int width)
	{
		const bool dither = true; // false = plain nearest-colour matching

		// ---------- load + scale ----------
		sf::Texture texture;
		if (!texture.loadFromFile(imagePath))
			return;
		sf::Vector2u texSize = texture.getSize();
		if (texSize.x == 0 || width <= 0)
			return;

		float scaleFactor = (float)width / (float)texSize.x;
		int newH = std::max(1, (int)std::round((float)texSize.y * scaleFactor));
		texture.setSmooth(false);

		sf::RenderTexture rt(sf::Vector2u{(unsigned)width, (unsigned)newH});
		rt.clear(sf::Color::Transparent);
		sf::Sprite sprite(texture);
		sprite.setScale({scaleFactor, scaleFactor});
		sf::RenderStates states;
		states.blendMode = sf::BlendNone;
		rt.draw(sprite, states);
		rt.display();
		sf::Image small = rt.getTexture().copyToImage();

		// ---------- sRGB -> linear lookup table ----------
		float srgbToLin[256];
		for (int i = 0; i < 256; i++)
		{
			float c = i / 255.f;
			srgbToLin[i] = c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
		}

		// ---------- tile averages (linear light) -> Lab ----------
		int tileCount = (int)TileInfo::tileRegistry.size();
		std::vector<float> tileLab(tileCount * 3, 0.f); // L,a,b per tile
		std::vector<float> tileRgb(tileCount * 3, 0.f); // average sRGB 0-255 (for dither error)
		std::vector<char> tileValid(tileCount, 0);

		for (int i = 0; i < tileCount; i++)
		{
			auto &t = TileInfo::tileRegistry[i];
			if (t.images.size() == 0)
				continue;
			if (t.isTileEntity)
			{
				continue;
			}
			sf::Image &im = t.images[0];
			float sum[3] = {0.f, 0.f, 0.f};
			int n = 0;
			for (unsigned y = 0; y < im.getSize().y; y++)
			{
				for (unsigned x = 0; x < im.getSize().x; x++)
				{
					sf::Color p = im.getPixel({x, y});
					if (p.a == 0)
						continue;
					n++;
					sum[0] += srgbToLin[p.r];
					sum[1] += srgbToLin[p.g];
					sum[2] += srgbToLin[p.b];
				}
			}
			if (n == 0)
				continue; // fully transparent tile, never matched
		
			float lin[3] = {sum[0] / n, sum[1] / n, sum[2] / n};

			// linear -> sRGB (stored so dithering can compute the error)
			for (int k = 0; k < 3; k++)
			{
				float c = lin[k];
				float s = c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1.f / 2.4f) - 0.055f;
				tileRgb[i * 3 + k] = s * 255.f;
			}

			// linear -> XYZ -> Lab
			float f[3];
			f[0] = (0.4124564f * lin[0] + 0.3575761f * lin[1] + 0.1804375f * lin[2]) / 0.95047f;
			f[1] = 0.2126729f * lin[0] + 0.7151522f * lin[1] + 0.0721750f * lin[2];
			f[2] = (0.0193339f * lin[0] + 0.1191920f * lin[1] + 0.9503041f * lin[2]) / 1.08883f;
			for (int k = 0; k < 3; k++)
				f[k] = f[k] > 0.008856f ? std::cbrt(f[k]) : 7.787f * f[k] + 16.f / 116.f;

			tileLab[i * 3 + 0] = 116.f * f[1] - 16.f;
			tileLab[i * 3 + 1] = 500.f * (f[0] - f[1]);
			tileLab[i * 3 + 2] = 200.f * (f[1] - f[2]);
			tileValid[i] = 1;
		}

		// ---------- working buffer (float, so dithering error can accumulate) ----------
		std::vector<float> buf(width * newH * 3);
		for (int y = 0; y < newH; y++)
		{
			for (int x = 0; x < width; x++)
			{
				sf::Color p = small.getPixel({(unsigned)x, (unsigned)y});
				int idx = (y * width + x) * 3;
				buf[idx + 0] = p.r;
				buf[idx + 1] = p.g;
				buf[idx + 2] = p.b;
			}
		}

		// ---------- match each pixel (row by row, needed for error diffusion) ----------
		for (int y = 0; y < newH; y++)
		{
			for (int x = 0; x < width; x++)
			{
				if (small.getPixel({(unsigned)x, (unsigned)y}).a == 0)
					continue;

				int idx = (y * width + x) * 3;

				// clamp, then convert this pixel to Lab
				float rgb[3];
				float lin[3];
				for (int k = 0; k < 3; k++)
				{
					rgb[k] = std::clamp(buf[idx + k], 0.f, 255.f);
					float c = rgb[k] / 255.f;
					lin[k] = c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
				}

				float f[3];
				f[0] = (0.4124564f * lin[0] + 0.3575761f * lin[1] + 0.1804375f * lin[2]) / 0.95047f;
				f[1] = 0.2126729f * lin[0] + 0.7151522f * lin[1] + 0.0721750f * lin[2];
				f[2] = (0.0193339f * lin[0] + 0.1191920f * lin[1] + 0.9503041f * lin[2]) / 1.08883f;
				for (int k = 0; k < 3; k++)
					f[k] = f[k] > 0.008856f ? std::cbrt(f[k]) : 7.787f * f[k] + 16.f / 116.f;

				float pL = 116.f * f[1] - 16.f;
				float pa = 500.f * (f[0] - f[1]);
				float pb = 200.f * (f[1] - f[2]);

				// nearest tile by squared Lab distance
				float bestDist = std::numeric_limits<float>::max();
				int best = -1;
				for (int i = 0; i < tileCount; i++)
				{
					if (!tileValid[i])
						continue;
					float dL = pL - tileLab[i * 3 + 0];
					float da = pa - tileLab[i * 3 + 1];
					float db = pb - tileLab[i * 3 + 2];
					float d = dL * dL + da * da + db * db;
					if (d < bestDist)
					{
						bestDist = d;
						best = i;
					}
				}
				if (best < 0)
					continue; // no usable tiles at all

				SetTileAt(placePos + sf::Vector2i{x, y}, Tile(best));

				// Floyd-Steinberg: push the leftover error onto unprocessed neighbours
				if (dither)
				{
					for (int k = 0; k < 3; k++)
					{
						float err = rgb[k] - tileRgb[best * 3 + k];
						if (x + 1 < width)
							buf[idx + 3 + k] += err * 7.f / 16.f;
						if (y + 1 < newH)
						{
							int below = idx + width * 3;
							if (x > 0)
								buf[below - 3 + k] += err * 3.f / 16.f;
							buf[below + k] += err * 5.f / 16.f;
							if (x + 1 < width)
								buf[below + 3 + k] += err * 1.f / 16.f;
						}
					}
				}
			}
		}
	}
}