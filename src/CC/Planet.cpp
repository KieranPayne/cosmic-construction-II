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
	}
	void Planet::VisibleUpdate(sf::RenderTarget *target, InputState &inputState, double dt)
	{
		camera.Update(dt, inputState);
		GenerateChunksInView(target);
		DrawInfoGUI(dt);
		DrawToolGUI(inputState);
		jsonEditor.Draw(this);
		// ImGui::Begin("test");
		// for (int i = 0; i < entities.size(); i ++)
		// {
		// 	nlohmann::json j = entities[i]->ToJson();
		// 	if (jsonEditor.Draw(j,("entity " +  std::to_string(i)).c_str()))
		// 	{
		// 		entities[i]->FromJson(j);
		// 	}		
		// }
		// ImGui::End();
	}
	void Planet::GenerateChunksInView(sf::RenderTarget *target)
	{
		sf::FloatRect view = camera.toFloatRect(target);
		constexpr int chunkSizePixels = CHUNK_SIZE * TILE_SIZE;
		sf::Vector2i topLeft = {(int)floor(view.position.x / chunkSizePixels), (int)floor(view.position.y / chunkSizePixels)};
		sf::Vector2i bottomRight = {(int)floor((view.position.x + view.size.x) / chunkSizePixels), (int)floor((view.position.y + view.size.y) / chunkSizePixels)};
		topLeft -= {1, 1};
		bottomRight += {1, 1};
		for (int x = topLeft.x; x <= bottomRight.x; x++)
		{
			for (int z = topLeft.y; z <= bottomRight.y; z++)
			{

				if (!chunks.contains({x, z}))
				{

					chunks[{x, z}] = std::unique_ptr<Chunk>(generator.GenerateChunk({x, z}));
				}
				sf::Vector2i pos(x, z);
			}
		}
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
		Serializer entityData(Serializer::Mode::WRITE,SaveManager::saveFormat);
		int n = entities.size();
		entityData.field("n",n);
		// nlohmann::json entityData;
		for (int i = 0; i < n; i ++)
		{
			uint16_t type = (uint16_t)entities[i]->type;
			entityData.field(std::to_string(i) + " type",type);
			entityData.field(std::to_string(i),entities[i].get());
		}
		SaveManager::WriteSerializerToFile(entityData, path + "/entities");
		// SaveManager::WriteData(path + "/entities.json", entityData.dump(2));
		// misc variables get saved in planet json
		Serializer s(Serializer::Mode::WRITE,SaveManager::saveFormat);
		Serialize(s);
		SaveManager::WriteSerializerToFile(s,path + "/planet");
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
		entityData.field("n",n);
		// nlohmann::json entityData = nlohmann::json::parse(SaveManager::ReadData(path + "/entities.json"));
		for (int i = 0; i < n; i ++)
		{
			uint16_t type;
			entityData.field(std::to_string(i) + " type",type);
			Entity* e = CreateEntityFromType((Entity::EntityType)type);
			entityData.field(std::to_string(i),e);
			AddEntity(e);
		}
		// load misc data
		Serializer s = SaveManager::LoadSerializerFromFile(path +"/planet");
		Serialize(s);
		// FromJson(nlohmann::json::parse(SaveManager::ReadData(path + "/planet.json")));
		generator.Load(path);
	}
	void Planet::AddEntity(Entity *entity)
	{
		sf::Vector2i chunkPos = TileToChunkPos(entity->position);
		entity->chunkPos = chunkPos;
		entities.push_back(std::unique_ptr<Entity>(entity));
		if (!chunks.contains(chunkPos))
		{
			chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		chunks[chunkPos]->AddEntity(entity);
	}
	void Planet::DrawInfoGUI(double dt)
	{
		if (macro.active)
		{
			return;
		}
		static int currentView = -1;
		ImGui::SetNextWindowPos(ImVec2(321,4),ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(362,183),ImGuiCond_Once);
		ImGui::Begin("World Info");
		double fps = 1.0 / (dt + 0.0000000001);
		currFps += (fps - currFps) * dt * 3;
		ImGui::Text(("FPS: " + std::to_string(currFps)).c_str());
		// ImGui::SliderInt("Tracking Entity", &trackingEntity, -1, entities.size() - 1);

		static float t = 0.1f;
		ImGui::SliderFloat("Time Per Tick", &t, 0.f, 1.f);
		((State *)state)->timePerTick = t;

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
			Serializer s(Serializer::Mode::WRITE,Serializer::Format::JSON);
			camera.Serialize(s);
			ImGui::Text(s.json().dump(2).c_str());
		}
		else if (currentView == 1)
		{
			std::string result = "";
			for (auto &e : entities)
			{
				Serializer s(Serializer::Mode::WRITE,Serializer::Format::JSON);
				e->Serialize(s);
				result += s.json().dump(2) + "\n";
			}
			ImGui::Text(result.c_str());
		}
		ImGui::End();
	}
	//TODO: make this return a pair that also returns the tile entity if there is one here
	std::pair<Tile*,TileEntity*> Planet::GetTileAt(sf::Vector2i position)
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
	void Planet::SetTileAt(sf::Vector2i position, Tile tile, TileEntity* tileEntity)
	{
		sf::Vector2i chunkPos = TileToChunkPos(position);
		if (!chunks.contains(chunkPos))
		{
			chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		sf::Vector2i subChunkPos = position - chunkPos * CHUNK_SIZE;
		chunks[chunkPos]->SetTile(subChunkPos,tile, tileEntity);
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
		ImGui::SetNextWindowPos(ImVec2(4,316),ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(231,400),ImGuiCond_Once);
		ImGui::Begin("Tool Menu");
		ImGui::Text("Hovering over:");
		std::string bgTile = "Background Tile:\n";
		sf::Vector2f worldPos = camera.ToWorldPos(inputState.mousePosition, window.get());
		sf::Vector2i worldTilePos(floor((float)worldPos.x / TILE_SIZE), floor((float)worldPos.y / TILE_SIZE));
		sf::Vector2i worldChunkPos = TileToChunkPos(worldTilePos);
		sf::Vector2i subChunkPos = worldTilePos - worldChunkPos * CHUNK_SIZE;
		BackgroundTile *bgT = &chunks[worldChunkPos]->backgroundTiles[subChunkPos.x][subChunkPos.y];
		bgTile += "Colour: " + std::to_string(bgT->color.r) + " " + std::to_string(bgT->color.g) + " " + std::to_string(bgT->color.b) + "\n";
		bgTile += "Type: " + std::to_string((uint8_t) bgT->type);
		ImGui::Text(bgTile.c_str());
		std::string tile = "Tile:\n";
		auto t = GetTileAt(worldTilePos);
		tile += "Type: " + TileInfo::tileRegistry[t.first->type].name;
		if (t.second != nullptr)
		{
			tile += "\nTile Entity data:\n";
			tile += t.second->ToJson().dump(2);
		}
		ImGui::Text(tile.c_str());
		static int currentView = 0;
		std::vector<std::string> names = {"Add Tile", "Add Entity", "blah blahh blahhhhoiahsdfoj"};
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
			}
		}
		else if (currentView == 1)
		{
			if (inputState.Pressed(sf::Mouse::Button::Right))
			{
				sf::Vector2f worldPos = camera.ToWorldPos(inputState.mousePosition, window.get());
				sf::Vector2i worldTilePos(floor((float)worldPos.x / TILE_SIZE), floor((float)worldPos.y / TILE_SIZE));
			}
		}
		ImGui::End();
	}
	void Planet::Serialize(Serializer& s)
	{
		s.field("camera",camera);
		s.field("seed",seed);
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
					c->tileEntities[c->TileEntityIndex({x,y})]->GetVertices(arr,i);
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
	
}