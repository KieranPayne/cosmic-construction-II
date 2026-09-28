#pragma once
#include "../PCH.hpp"
#include "Camera.hpp"
#include "Chunk.hpp"
#include "Generator.hpp"
#include "../json.hpp"
#include "Entity.hpp"
#include "TileEntity.hpp"
#include "JsonEditor.hpp"
#include "Serializer.hpp"
namespace cc
{
	class Planet
	{
	public:
		std::vector<std::unique_ptr<Entity>> entities;
		std::unordered_map<sf::Vector2i, std::unique_ptr<Chunk>, ChunkHash> chunks;
		std::unordered_map<sf::Vector2i, sf::VertexArray, ChunkHash> tileVertices;
		Generator generator;
		Camera camera;
		int index;
		uint64_t seed = 0;
		JsonEditor jsonEditor;

		// for display
		double currFps;

		Planet();
		void VisibleUpdate(sf::RenderTarget *target, InputState &inputState, double dt);
		void Update(double dt);
		void Tick();
		std::vector<sf::Vector2i> GetChunksToRequest(sf::RenderTarget *target);
		void Render(sf::RenderTarget *target);
		void DrawInfoGUI(double dt);
		void DrawToolGUI(InputState &inputState);
		void Save();
		void Load();
		void GetTileVertices(sf::Vector2i chunkPos);
		//NOTE: THIS IS MOVING CHUNKS, NOT JUST MOVING IN GENERAL.
		void MoveEntity(Entity* entity, sf::Vector2i newPos);
		void AddEntity(Entity* entity);
		// like with the layerVertices map, the pos x and z are chunk coordinates, but the y is view height.
		void GenerateLayerVertices(sf::Vector2i pos);
		std::pair<std::vector<sf::Vertex>, bool> GetVertices(sf::Vector2i tilePosition);
		std::pair<Tile*,TileEntity*> GetTileAt(sf::Vector2i position);
		void SetTileAt(sf::Vector2i position, Tile tile, TileEntity* tileEntity = nullptr);
		void Serialize(Serializer& s);
		// nlohmann::json ToJson();
		// void FromJson(nlohmann::json j);
		void SetSeed(uint64_t seed);
		void GenerateChunk(sf::Vector2i position);
		// some nonsense to allow having a vector of unique ptrs
		Planet(const Planet &) = delete;
		Planet &operator=(const Planet &) = delete;
		Planet(Planet &&) = default;
		Planet &operator=(Planet &&) = default;
	};
}