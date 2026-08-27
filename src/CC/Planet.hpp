#pragma once
#include "../PCH.hpp"
#include "Camera.hpp"
#include "Chunk.hpp"
#include "Generator.hpp"
#include "../json.hpp"
namespace cc
{
	class Planet
	{
	public:
		std::unordered_map<sf::Vector2i, std::unique_ptr<Chunk>,ChunkHash> chunks;
		std::unordered_map<sf::Vector2i, sf::VertexArray,ChunkHash> tileVertices;
		Generator generator;
		Camera camera;
		int index;
		int viewDepth = 10;
		int overlayAlpha = 76;
		uint64_t seed = 0;

		//for display
		double currFps;

		Planet();
		void VisibleUpdate(sf::RenderTarget *target, InputState &inputState, double dt);
		void Update(double dt);
		void Tick();
		void GenerateChunksInView(sf::RenderTarget *target);
		void Render(sf::RenderTarget *target);
		void DrawInfoGUI(double dt);
		void DrawToolGUI(InputState& inputState);
		void Save();
		void Load();
		void GetTileVertices (sf::Vector2i chunkPos);
		//like with the layerVertices map, the pos x and z are chunk coordinates, but the y is view height.
		void GenerateLayerVertices(sf::Vector2i pos);
		std::pair<std::vector<sf::Vertex>,bool> GetVertices(sf::Vector2i tilePosition);
		Tile *GetTileAt(sf::Vector2i position);
		void SetTileAt(sf::Vector2i position, Tile tile);
		nlohmann::json ToJson();
		void FromJson(nlohmann::json j);
		void SetSeed(uint64_t seed);
		// some nonsense to allow having a vector of unique ptrs
		Planet(const Planet &) = delete;
		Planet &operator=(const Planet &) = delete;
		Planet(Planet &&) = default;
		Planet &operator=(Planet &&) = default;
	};
}