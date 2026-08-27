#include "Planet.hpp"
#include "../Timer.hpp"
#include "PerlinNoise.hpp"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "TileInfo.hpp"
#include "../imgui/imgui.h"
#include "../Main.hpp"
#include <queue>
namespace cc
{
	Planet::Planet()
	{
		index = -1;
		// chunks.clear();
		// camera = Camera();
		currFps = 0;
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
	}
	void Planet::GenerateChunksInView(sf::RenderTarget *target)
	{
		sf::FloatRect view = camera.toFloatRect(target);
		constexpr int chunkSizePixels = CHUNK_SIZE * TILE_SIZE;
		sf::Vector2i topLeft = {(int)floor(view.position.x / chunkSizePixels), (int)floor(view.position.y / chunkSizePixels)};
		sf::Vector2i bottomRight = {(int)floor((view.position.x + view.size.x) / chunkSizePixels), (int)floor((view.position.y + view.size.y) / chunkSizePixels)};
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
		for (int x = topLeft.x; x <= bottomRight.x; x ++)
		{
			for (int y = topLeft.y; y <= bottomRight.y; y ++)
			{
				if (!tileVertices.contains({x,y}))
				{
					GetTileVertices({x,y});
				}
				target->draw(tileVertices[{x,y}],states);
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
			auto bytes = c.second->ToBytes();
			std::string chunkPath = path + "/chunks/";
			chunkPath += std::to_string(c.second->position.x) + " " + std::to_string(c.second->position.y);
			std::ofstream out(chunkPath + ".txt", std::ios::binary);
			out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
			out.close();
		}
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
			coords[1] = coords[1].substr(0, coords[1].size() - 4);
			sf::Vector2i vec{std::stoi(coords[0]), std::stoi(coords[1])};
			std::array<uint8_t, CHUNK_NUM_BYTES> bytes = {};
			std::ifstream in(f, std::ios::binary);
			in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
			chunks[vec] = std::make_unique<Chunk>(vec);
			chunks[vec]->FromBytes(bytes);
		}
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
		// ImGui::SliderInt("Tracking Entity", &trackingEntity, -1, entities.size() - 1);
		
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
			ImGui::Text(result.c_str());
		}else if(currentView == 2){
			std::string result = "";
			ImGui::Text(result.c_str());
		}
		ImGui::End();
	}
	Tile *Planet::GetTileAt(sf::Vector2i position)
	{
		sf::Vector2i chunkPos = TileToChunkPos(position);
		if (!chunks.contains(chunkPos))
		{
			chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		sf::Vector2i subChunkPos = position - chunkPos * CHUNK_SIZE;
		return &chunks[chunkPos]->tiles[subChunkPos.x][subChunkPos.y];
	}
	void Planet::SetTileAt(sf::Vector2i position, Tile tile)
	{
		sf::Vector2i chunkPos = TileToChunkPos(position);
		if (!chunks.contains(chunkPos))
		{
			chunks[chunkPos] = std::unique_ptr<Chunk>(generator.GenerateChunk(chunkPos));
		}
		sf::Vector2i subChunkPos = position - chunkPos * CHUNK_SIZE;
		chunks[chunkPos]->tiles[subChunkPos.x][subChunkPos.y] = tile;
		if (tileVertices.contains(chunkPos))
		{
			//erase the vertices since they're no longer accurate
			//note: erase instead of rebuilding since these may not necessarily be visible, so this would be wasted effort.
			tileVertices.erase(chunkPos);
		}
	}
	void Planet::Tick()
	{
	}
	std::pair<std::vector<sf::Vertex>,bool> Planet::GetVertices(sf::Vector2i tilePosition)
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
			sf::Vector2f texPos = (sf::Vector2f)TileInfo::tileRegistry[t->type].position;
			for (int i = 0; i < 6; i++)
			{
				vertices.push_back({worldPos + offsets[i], sf::Color(alpha, alpha, alpha, 255), texPos + offsets[i]});
			}
			return {vertices,true};
		}
		return {{},false};
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
	nlohmann::json Planet::ToJson(){
		nlohmann::json j;
		j["overlayAlpha"] = overlayAlpha;
		j["viewDepth"] = viewDepth;
		j["camera"] = camera.ToJson();
		j["seed"] = seed;
		j["generator"] = generator.ToJson();
		return j;
	}
	void Planet::FromJson(nlohmann::json j){
		overlayAlpha = j["overlayAlpha"];
		viewDepth = j["viewDepth"];
		camera.FromJson(j["camera"]);
		generator.FromJson(j["generator"]);
	}
	void Planet::GetTileVertices (sf::Vector2i chunkPos)
	{
		sf::VertexArray arr(sf::PrimitiveType::Triangles,CHUNK_SIZE * CHUNK_SIZE * 6 * 2);
		//get background tile vertices
		static sf::Vector2f offsets[6] = {
		{0, 0},
		{TILE_SIZE, 0},
		{TILE_SIZE, TILE_SIZE},
		{0, 0},
		{TILE_SIZE, TILE_SIZE},
		{0, TILE_SIZE}};
		int i = 0; 
		Chunk* c = chunks[chunkPos].get();
		for (int x = 0; x < CHUNK_SIZE; x ++)
		{
			for (int y = 0; y < CHUNK_SIZE; y ++)
			{
				for (int j = 0; j < 6; j ++)
				{
					sf::Vertex v;
					v.position = (sf::Vector2f)(chunkPos * CHUNK_SIZE * TILE_SIZE) + sf::Vector2f{x * TILE_SIZE,y * TILE_SIZE} + offsets[j];
					v.color = c->backgroundTiles[x][y].color;
					
					arr[i] = v;
					i ++; 
				}	
			}
		}
		for (int x = 0; x < CHUNK_SIZE; x ++)
		{
			for (int y = 0; y < CHUNK_SIZE; y ++)
			{
				Tile* t = &c->tiles[x][y];

				//air tile; forgive the magic number pls
				if (t->type == 0)
				{
					continue;
				}
				sf::Vector2f texPos = (sf::Vector2f)TileInfo::tileRegistry[t->type].position;
				for (int j = 0; j < 6; j ++)
				{
					sf::Vertex v;
					v.position = (sf::Vector2f)(chunkPos * CHUNK_SIZE * TILE_SIZE) + sf::Vector2f{x * TILE_SIZE,y * TILE_SIZE} + offsets[j];
					v.texCoords = texPos + offsets[j];
					arr[i] = v;
					i ++; 
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