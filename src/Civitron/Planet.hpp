#pragma once
#include "../PCH.hpp"
#include "Camera.hpp"
#include "Chunk.hpp"
#include "Generator.hpp"
#include "Entity.hpp"
#include "../json.hpp"
#include "Population.hpp"
namespace Civitron
{
	class Planet
	{
	public:
		std::vector<std::unique_ptr<Entity>> entities = {};
		std::unordered_map<sf::Vector2i, std::unique_ptr<Chunk>,ChunkHash> chunks;
		//note that x and z are in chunk coordinates, whereas y is view height
		//bool indicates the layer is solid
		Generator generator;
		Camera camera;
		int index;
		int trackingEntity = -1;
		int viewDepth = 10;
		int overlayAlpha = 76;

		//for display
		double currFps;

		Planet();
		void SetSeed(uint64_t seed);
		void VisibleUpdate(sf::RenderTarget *target, InputState &inputState, double dt);
		void Update(double dt);
		void Tick();
		void GenerateChunksInView(sf::RenderTarget *target);
		void Render(sf::RenderTarget *target);
		void DrawInfoGUI(double dt);
		void DrawToolGUI(InputState& inputState);
		void Save();
		void Load();
		void AddEntity(Entity *e);
		void RemoveEntity(Entity* e);
		//like with the layerVertices map, the pos x and z are chunk coordinates, but the y is view height.
		void GenerateLayerVertices(sf::Vector2i pos);
		bool TileIsWalkable(sf::Vector2i pos);
		std::pair<std::vector<sf::Vertex>,bool> GetVertices(sf::Vector2i tilePosition);
		Tile *GetTileAt(sf::Vector2i position);
		void SetTileAt(sf::Vector2i position, Tile tile);
		nlohmann::json ToJson();
		void FromJson(nlohmann::json j);
		// some nonsense to allow having a vector of unique ptrs
		Planet(const Planet &) = delete;
		Planet &operator=(const Planet &) = delete;
		Planet(Planet &&) = default;
		Planet &operator=(Planet &&) = default;
	};
}