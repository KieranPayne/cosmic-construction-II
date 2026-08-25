#include "Planet.hpp"
#include "../Timer.hpp"
#include "PerlinNoise.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "EntityInfo.hpp"
#include "TileInfo.hpp"
#include "../imgui/imgui.h"
#include "../Main.hpp"
#include "Human.hpp"
#include <queue>
namespace Civitron
{
	Planet::Planet()
	{
		index = -1;
		chunks.clear();
		camera = Camera();
		currFps = 0;
	}
	void Planet::SetSeed(uint64_t seed)
	{
		generator.SetSeed(seed);
	}
	void Planet::Update(double dt)
	{
	}
	void Planet::VisibleUpdate(sf::RenderTarget *target, InputState &inputState, double dt)
	{
		int prevHeight = camera.viewHeight;
		if (inputState.Down(sf::Keyboard::Key::LControl))
		{
			if (inputState.scroll.y > 0)
			{
				camera.viewHeight++;
			}
			else if (inputState.scroll.y < 0)
			{
				camera.viewHeight--;
			}
		}
		if (inputState.Pressed(sf::Keyboard::Key::E))
		{
			camera.viewHeight++;
		}
		if (inputState.Pressed(sf::Keyboard::Key::Q))
		{
			camera.viewHeight--;
		}
		bool changedViewHeight = prevHeight != camera.viewHeight;
		while (trackingEntity >= (int)entities.size()){
			trackingEntity --;
		}
		if (trackingEntity != -1)
		{
			Entity *e = entities[trackingEntity].get();
			sf::Vector2f targetPos = sf::Vector2f((e->position.x + 0.5f) * TILE_SIZE, (e->position.z + 0.5f) * TILE_SIZE);
#define LERP(a, b, t) (a) + ((b) - (a)) * (t)
			camera.position = sf::Vector2f(LERP(camera.position.x, targetPos.x, dt * 10), LERP(camera.position.y, targetPos.y, dt * 10));
#undef LERP
			camera.viewHeight = e->position.y;
		}
		camera.Update(dt, inputState);
		GenerateChunksInView(target, changedViewHeight);
		DrawInfoGUI(dt);
		DrawToolGUI(inputState);
	}
	void Planet::GenerateChunksInView(sf::RenderTarget *target, bool changedViewHeight)
	{
		sf::FloatRect view = camera.toFloatRect(target);
		constexpr int chunkSizePixels = CHUNK_SIZE * TILE_SIZE;
		sf::Vector2i topLeft = {(int)floor(view.position.x / chunkSizePixels), (int)floor(view.position.y / chunkSizePixels)};
		sf::Vector2i bottomRight = {(int)floor((view.position.x + view.size.x) / chunkSizePixels), (int)floor((view.position.y + view.size.y) / chunkSizePixels)};
		int y = floor((float)camera.viewHeight / CHUNK_SIZE);
		for (int x = topLeft.x; x <= bottomRight.x; x++)
		{
			for (int z = topLeft.y; z <= bottomRight.y; z++)
			{

				for (int i = 0; i <= viewDepth; i++)
				{
					int y = camera.viewHeight - i;
					int chunkY = floor((float)y / CHUNK_SIZE);
					if (!chunks.contains({x, chunkY, z}))
					{

						chunks[{x, chunkY, z}] = std::unique_ptr<Chunk>(generator.GenerateChunk({x, chunkY, z}));
					}
					sf::Vector3i pos(x, y, z);
					if (!layerVertices.contains(pos))
					{
						GenerateLayerVertices(pos);
					}else{
						auto index = std::find(verticesToRedraw.begin(),verticesToRedraw.end(),pos); 
						if (index != verticesToRedraw.end()){
							GenerateLayerVertices(pos);
							verticesToRedraw.erase(index);
						}
					}
				}
			}
		}
	}
	void Planet::Render(sf::RenderTarget *target)
	{
		// std::vector<Entity *> entitiesToRender = {};
		sf::RenderStates states;
		states.texture = &TileInfo::atlas.texture;
		sf::RenderStates entityStates;
		entityStates.texture = &EntityInfo::atlas.texture;
		sf::FloatRect view = camera.toFloatRect(target);
		constexpr int chunkSizePixels = CHUNK_SIZE * TILE_SIZE;
		sf::Vector2i topLeft = {(int)floor(view.position.x / chunkSizePixels), (int)floor(view.position.y / chunkSizePixels)};
		sf::Vector2i bottomRight = {(int)floor((view.position.x + view.size.x) / chunkSizePixels), (int)floor((view.position.y + view.size.y) / chunkSizePixels)};
		int y = floor((float)camera.viewHeight / CHUNK_SIZE);
		sf::FloatRect rect = camera.toFloatRect(window.get());
		sf::VertexArray overlay(sf::PrimitiveType::Triangles, 6);
		overlay[0].position = rect.position;
		overlay[1].position = rect.position + sf::Vector2f(rect.size.x, 0);
		overlay[2].position = rect.position + rect.size;
		overlay[3].position = rect.position;
		overlay[4].position = rect.position + rect.size;
		overlay[5].position = rect.position + sf::Vector2f(0, rect.size.y);
		for (int i = 0; i < 6; i++)
		{
			overlay[i].color = sf::Color(0, 0, 0, overlayAlpha);
		}
		std::vector<std::vector<int>> heights;
		for (int x = topLeft.x; x <= bottomRight.x; x++)
		{
			heights.push_back({});
			for (int z = topLeft.y; z <= bottomRight.y; z++)
			{
				for (int y = 0; y <= viewDepth; y++)
				{
					if (layerVertices[sf::Vector3i(x, camera.viewHeight - y, z)].second || y == viewDepth)
					{
						heights.back().push_back(viewDepth-y);
						break;
					}
				}
			}
		}
		for (int i = 0; i <= viewDepth; i++)
		{
			int y = camera.viewHeight - viewDepth + i;
			window->draw(overlay, states);
			std::vector<sf::Vertex> entityVerts = {};
			for (int x = topLeft.x; x <= bottomRight.x; x++)
			{
				for (int z = topLeft.y; z <= bottomRight.y; z++)
				{
					//TODO: LINE BELOW MADE CHUNKS DISAPPEAR
					sf::Vector3i cPos(x,TileToChunkPos(y),z);
					if (chunks.contains(cPos)){
						Chunk* c = chunks[cPos].get();
						for (auto& e : c->entities){
							auto verts = e->GetVertices(this,y);
							entityVerts.insert(entityVerts.end(),verts.begin(),verts.end());
						}
					}

					if (i < heights[x - topLeft.x][z - topLeft.y])
					{
						continue;
					}
					sf::Vector3i pos(x,y,z);
					auto &verts = layerVertices[pos].first;
					window->draw(verts, states);
				}
			}
			window->draw(entityVerts.data(),entityVerts.size(),sf::PrimitiveType::Triangles,entityStates);
		}
		// sf::VertexArray arr(sf::PrimitiveType::Triangles);
		// for (Entity *e : entitiesToRender)
		// {
		// 	sf::VertexArray arr2 = e->GetVertices(this);
		// 	for (int i = 0; i < arr2.getVertexCount(); i++)
		// 	{
		// 		arr.append(arr2[i]);
		// 	}
		// }
		// sf::RenderStates s;
		// s.texture = &EntityInfo::atlas.texture;
		// target->draw(arr, s);
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
			auto bytes = c.second->ToBytes();
			std::string chunkPath = path + "/chunks/";
			chunkPath += std::to_string(c.second->position.x) + " " + std::to_string(c.second->position.y) + " " + std::to_string(c.second->position.z);
			std::ofstream out(chunkPath + ".txt");
			out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
			out.close();
		}
		//save entities
		std::string entityData = "[\n";
		for (auto &e : entities)
		{
			auto j = e->ToJson();
			entityData += j.dump(2);
			// separator
			entityData += ",\n";
		}
		if (entityData.size() > 2){
			entityData = entityData.substr(0, entityData.size() - 2);
		}
		entityData += "\n]";
		SaveManager::WriteData(path + "/entities.json", entityData);
		//save populations
		std::string populationData = "[\n";
		for (auto& p : populations){
			populationData += p.ToJson().dump(2) + ",\n";
		}
		if (populationData.size() > 2){
			populationData = populationData.substr(0, populationData.size() - 2);
		}
		populationData += "\n]";
		SaveManager::WriteData(path + "/populations.json", populationData);
		//save generator data
		std::string generatorPath = path + "/generator";
		if (!SaveManager::DirExists(generatorPath))
		{
			SaveManager::CreateDirectory(generatorPath);
		}
		generatorPath += "/";
		generator.Save(generatorPath);
		//misc variables get saved in planet json
		SaveManager::WriteData(path + "/planet.json",ToJson().dump(2));
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
			coords[2] = coords[2].substr(0, coords[2].size() - 4);
			sf::Vector3i vec{std::stoi(coords[0]), std::stoi(coords[1]), std::stoi(coords[2])};
			std::array<uint8_t, CHUNK_NUM_BYTES> bytes = {};
			std::ifstream in(f, std::ios::binary);
			in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
			chunks[vec] = std::make_unique<Chunk>(vec);
			chunks[vec]->FromBytes(bytes);
		}
		// load entities
		std::string entityData = SaveManager::ReadData(path + "/entities.json");
		nlohmann::json j = nlohmann::json::parse(entityData);
		for (auto &json : j)
		{
			Entity *e = Entity::LoadFromJson(json);
			AddEntity(e);
		}
		//load populations
		std::string populationData = SaveManager::ReadData(path + "/populations.json");
		j = nlohmann::json::parse(populationData);
		for (auto &json : j)
		{
			Population p;
			p.FromJson(json);
			populations.push_back(p);
		}

		std::string generatorPath = path + "/generator/";
		generator.Load(generatorPath);
		// load misc data
		FromJson(nlohmann::json::parse(SaveManager::ReadData(path + "/planet.json")));
	}
	void Planet::DrawInfoGUI(double dt)
	{
		if (macro.active)
		{
			return;
		}
		static int currentView = -1;
		ImGui::Begin("World Info");
		double fps = 1.0 / (dt + 0.0000000001);
		currFps += (fps - currFps) * dt * 3;
		ImGui::Text(("FPS: " + std::to_string(currFps)).c_str());
		ImGui::SliderInt("Tracking Entity", &trackingEntity, -1, entities.size() - 1);
		
		ImGui::SliderInt("Overlay Alpha", &overlayAlpha, 0, 255);
		ImGui::SliderInt("View Depth", &viewDepth, 0, 15);
		static float t = 0.1f;
		ImGui::SliderFloat("Time Per Tick",&t, 0.f, 1.f);
		((State*)state)->timePerTick = t;
		const char *currentLabel = "None";
		switch (currentView)
		{
		case 0:
			currentLabel = "Camera";
			break;
		case 1:
			currentLabel = "Entities";
			break;
		case 2:
			currentLabel = "Populations";
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
			if (ImGui::Selectable("Populations", currentView == 2))
				currentView = 2;
			ImGui::EndCombo();
		}
		ImGui::Separator();
		if (currentView == 0)
		{
			ImGui::Text(camera.ToJson().dump(2).c_str());
		}
		else if (currentView == 1)
		{
			std::string result = "";
			for (auto &e : entities)
			{
				result += e->ToJson().dump(1);
				result += "\n";
			}
			ImGui::Text(result.c_str());
		}else if(currentView == 2){
			std::string result = "";
			for (auto &p : populations)
			{
				result += p.ToJson().dump(1);
				result += "\n";
			}
			ImGui::Text(result.c_str());
		}
		ImGui::End();
	}
	void Planet::AddEntity(Entity *e)
	{
		e->planet = this;
		entities.push_back(std::unique_ptr<Entity>(e));
		auto pos = TileToChunkPos(e->position);
		if (!chunks.contains(pos))
		{
			chunks[pos] = std::unique_ptr<Chunk>(generator.GenerateChunk(pos));
		}
		chunks[pos]->entities.push_back(e);
	}
	void Planet::RemoveEntity(Entity* e){
		auto chunkPos = TileToChunkPos(e->position);
		auto& c = chunks[chunkPos];
		c->RemoveEntity(e);
		e->toBeDeleted = true;
	}
	Tile *Planet::GetTileAt(sf::Vector3i position)
	{
		sf::Vector3i chunkPos = TileToChunkPos(position);
		if (!chunks.contains(chunkPos))
		{
			chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		sf::Vector3i subChunkPos = position - chunkPos * CHUNK_SIZE;
		return &chunks[chunkPos]->tiles[subChunkPos.x][subChunkPos.y][subChunkPos.z];
	}
	void Planet::SetTileAt(sf::Vector3i position, Tile tile)
	{
		sf::Vector3i chunkPos = TileToChunkPos(position);
		if (!chunks.contains(chunkPos))
		{
			chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		sf::Vector3i subChunkPos = position - chunkPos * CHUNK_SIZE;
		chunks[chunkPos]->tiles[subChunkPos.x][subChunkPos.y][subChunkPos.z] = tile;
		sf::Vector3i p(chunkPos.x,position.y, chunkPos.z);
		
		if (std::find(verticesToRedraw.begin(),verticesToRedraw.end(),p) == verticesToRedraw.end()){
			verticesToRedraw.push_back(p);
		}
	}
	void Planet::Tick()
	{
		for (int i = 0; i < entities.size(); i ++)
		{
			if (entities[i]->toBeDeleted){
				entities.erase(entities.begin() + i);
				i --;
			}else{
				entities[i]->Tick(this);
			}
		}
	}
	std::pair<std::vector<sf::Vertex>,bool> Planet::GetVertices(sf::Vector3i tilePosition)
	{
		sf::Vector3i chunkPos = TileToChunkPos(tilePosition);
		Chunk *chunk = chunks[chunkPos].get();
		tilePosition -= chunkPos * CHUNK_SIZE;
		float alpha = 255.f;
		static float falloff = 0.85f;
		Tile *t = &chunk->tiles[tilePosition.x][tilePosition.y][tilePosition.z];
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
			sf::Vector2f worldPos = {(float)(tilePosition.x * TILE_SIZE), (float)(tilePosition.z * TILE_SIZE)};
			sf::Vector2f texPos = (sf::Vector2f)TileInfo::tileRegistry[t->type].position;
			for (int i = 0; i < 6; i++)
			{
				vertices.push_back({worldPos + offsets[i], sf::Color(alpha, alpha, alpha, 255), texPos + offsets[i]});
			}
			bool solid = TileInfo::tileRegistry[t->type].solid;
			return {vertices,solid};
		}
		return {{},false};
	}
	bool Planet::TileIsWalkable(sf::Vector3i pos)
	{
		return GetTileAt(pos)->type == GetTileID("Air") && TileInfo::tileRegistry[GetTileAt(pos - sf::Vector3i(0, 1, 0))->type].walkable;
	}

	void Planet::GenerateLayerVertices(sf::Vector3i pos)
	{
		sf::Vector3i p = pos;
		pos.x *= CHUNK_SIZE;
		pos.z *= CHUNK_SIZE;
		// sf::VertexArray arr(sf::PrimitiveType::Triangles, CHUNK_SIZE * CHUNK_SIZE * 6);
		std::vector<sf::Vertex> arr;
		arr.reserve(CHUNK_SIZE * CHUNK_SIZE * 6);
		sf::Vector2f offset(pos.x * TILE_SIZE, pos.z * TILE_SIZE);
		int index = 0;
		bool solid = true;
		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				auto [vertices, tileSolid] = GetVertices(pos + sf::Vector3i(x, 0, z));
				solid = solid && tileSolid;
				for (int i = 0; i < vertices.size(); i++)
				{
					vertices[i].position += offset;
					arr.push_back(vertices[i]);
				}
				index += vertices.size();
			}
		}
		layerVertices[p] = std::pair<sf::VertexBuffer, bool>(sf::VertexBuffer(sf::PrimitiveType::Triangles), solid);
		auto &buff = layerVertices[p].first;
		buff.create(arr.size());
		buff.update(arr.data());
	}
	void Planet::DrawToolGUI(InputState &inputState)
	{
		ImGui::Begin("Tool Menu");
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
				SetTileAt(sf::Vector3i(worldTilePos.x, camera.viewHeight, worldTilePos.y), Tile(currentTile));
			}
		}
		else if (currentView == 1)
		{
			if (inputState.Pressed(sf::Mouse::Button::Right))
			{
				sf::Vector2f worldPos = camera.ToWorldPos(inputState.mousePosition, window.get());
				sf::Vector2i worldTilePos(floor((float)worldPos.x / TILE_SIZE), floor((float)worldPos.y / TILE_SIZE));
				Human *h = new Human();
				AddEntity(h);
				h->MoveTo(sf::Vector3i(worldTilePos.x, camera.viewHeight, worldTilePos.y));
			}
		}
		ImGui::End();
	}
	nlohmann::json Planet::ToJson(){
		nlohmann::json j;
		j["trackingEntity"] = trackingEntity;
		j["overlayAlpha"] = overlayAlpha;
		j["viewDepth"] = viewDepth;
		j["camera"] = camera.ToJson();
		return j;
	}
	void Planet::FromJson(nlohmann::json j){
		trackingEntity = j["trackingEntity"];
		overlayAlpha = j["overlayAlpha"];
		viewDepth = j["viewDepth"];
		camera.FromJson(j["camera"]);
	}
}