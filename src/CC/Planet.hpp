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
	class Client;

	/**
	 * @brief One planet: its chunks, tiles and entities, and the code to draw and edit them.
	 *
	 * The same class is used on both sides of the network. The server's planet (`isServerPlanet` is true) is the
	 * authority: it generates chunks when they are needed. A client's planet only holds the chunks the server
	 * has sent, asks for the ones it is missing, and sends its changes to the server through `client`.
	 */
	class Planet
	{
	public:
		/// Every entity on the planet. The index in this vector is the entity's id in update packets. The planet owns them.
		std::vector<std::unique_ptr<Entity>> entities;

		/// The chunks that currently exist, keyed by chunk position (in chunks, not tiles). On a client these are only the chunks received so far.
		std::unordered_map<sf::Vector2i, std::unique_ptr<Chunk>, ChunkHash> chunks;

		/// Drawing data for each chunk's tiles, keyed by chunk position. Built when a chunk is first drawn
		/// and thrown away when one of its tiles changes.
		std::unordered_map<sf::Vector2i, sf::VertexArray, ChunkHash> tileVertices;

		/// Creates the terrain for chunks that don't exist yet.
		Generator generator;

		/// The view onto the planet: position, zoom and movement.
		Camera camera;

		/// This planet's index in the server's list of planets. -1 until set. Names the planet's save folder.
		int index;

		/// The seed the terrain is generated from. Set with SetSeed().
		uint64_t seed = 0;

		/// The "Entity Editor" window.
		JsonEditor jsonEditor;

		/// The connection to the server, used to send changes. Null on the server's own planet.
		Client *client = nullptr;

		/// True for the server's planet, which generates missing chunks itself instead of waiting for them to be sent.
		bool isServerPlanet = false;

		/// Tile changes (position, tile and optional tile entity) waiting for their chunk to arrive. Client only.
		/// Update() applies each one once its chunk exists.
		std::vector<std::pair<sf::Vector2i, std::pair<Tile, TileEntity *>>> tileSetRequests;

		/// Chunk positions already asked for from the server, so they aren't asked for again.
		std::unordered_set<sf::Vector2i, ChunkHash> chunksRequested;

		/// Tile changes made on this client that have not been sent yet. Update() sends them all in one
		/// REQUEST_SET_TILES packet and clears this.
		std::vector<std::pair<sf::Vector2i, std::pair<Tile, TileEntity *>>> tilesToSend;

		/// Smoothed frames per second, shown in the info window.
		double currFps;

		/// @brief Creates an empty planet, with `index` of -1 and the generator linked to it.
		Planet();

		/**
		 * @brief Runs one frame for the planet being looked at: moves the camera and draws the planet's windows.
		 * @param target The window being drawn to. Not currently used.
		 * @param inputState The current mouse and keyboard state.
		 * @param dt Seconds since the previous frame.
		 */
		void VisibleUpdate(sf::RenderTarget *target, InputState &inputState, double dt);

		/**
		 * @brief Runs the per-frame bookkeeping.
		 *
		 * On the server: puts entities that aren't in a chunk yet into one, generating the chunk if needed.
		 * Everywhere: applies any waiting `tileSetRequests` whose chunk has arrived, and sends `tilesToSend` to the server.
		 * @param dt Seconds since the previous call. Not currently used.
		 */
		void Update(double dt);

		/// @brief Advances the planet by one fixed step: ticks every entity, then moves the ones that crossed into another chunk.
		void Tick();

		/**
		 * @brief Works out which chunks the client needs to ask the server for.
		 *
		 * Includes the chunks in view (plus a one chunk border), the chunks of entities not yet in a chunk,
		 * and the chunks of waiting `tileSetRequests`. Chunks that already exist or were already requested are left
		 * out, and the rest are recorded in `chunksRequested`.
		 *
		 * Also puts entities into their chunk if it has arrived.
		 * @param target The window being drawn to; its size and the camera decide what is in view.
		 * @return The chunk positions to request.
		 */
		std::vector<sf::Vector2i> GetChunksToRequest(sf::RenderTarget *target);

		/**
		 * @brief Draws the visible chunks' tiles, then their entities.
		 * @param target The window or texture to draw to.
		 */
		void Render(sf::RenderTarget *target);

		/**
		 * @brief Draws the "World Info" window: the frame rate, and optionally the camera's or entities' data as json.
		 * @param dt Seconds since the previous frame. Used for the frame rate.
		 */
		void DrawInfoGUI(double dt);

		/**
		 * @brief Draws the "Tool Menu" window.
		 *
		 * Shows the background tile, tile and tile entity under the mouse. Its tools are placing tiles
		 * (hold the right mouse button) and turning an image into tiles.
		 * @param inputState The current mouse and keyboard state.
		 */
		void DrawToolGUI(InputState &inputState);

		/**
		 * @brief Saves the planet into its folder in the current save (`SaveManager::savePath`).
		 *
		 * Writes every chunk, the entities, the planet's own data (see Serialize()), and the generator.
		 */
		void Save();

		/**
		 * @brief Loads the planet saved by Save(). Does nothing if there is no saved data for it.
		 *
		 * Entities are added as if the server sent them, so nothing is sent over the network.
		 */
		void Load();

		/**
		 * @brief Builds the drawing data for a chunk's tiles and stores it in `tileVertices`.
		 * @param chunkPos The chunk's position in chunk coordinates. The chunk must exist.
		 */
		void GetTileVertices(sf::Vector2i chunkPos);

		/**
		 * @brief Moves an entity from the chunk it is in to another chunk.
		 * @param entity The entity to move.
		 * @param newPos The destination chunk, in chunk coordinates (not tiles).
		 * @note This is about changing chunks, not about moving around in general.
		 *       On the server a missing destination chunk is generated; on a client the entity is
		 *       marked as not in a chunk.
		 */
		void MoveEntity(Entity *entity, sf::Vector2i newPos);

		/**
		 * @brief Adds an entity to the planet.
		 *
		 * The planet takes ownership. If the entity's chunk isn't loaded yet it is kept and put into the chunk later.
		 * @param entity The entity to add.
		 * @param sentByServer Pass true when the entity came from the server (or this is the server), so it is
		 *                     not sent back. When false, an add request is sent to the server through `client`.
		 */
		void AddEntity(Entity *entity, bool sentByServer = false);

		/**
		 * @brief Gets the drawing data for a single tile.
		 * @param tilePosition The tile's position in tiles. Its chunk must exist.
		 * @return The tile's vertices and true, or an empty list and false if the tile is air.
		 * @note Render() doesn't use this; it uses GetTileVertices().
		 */
		std::pair<std::vector<sf::Vertex>, bool> GetVertices(sf::Vector2i tilePosition);

		/**
		 * @brief Gets the tile at a position, and its tile entity if it has one.
		 * @param position The tile's position in tiles.
		 * @return The tile and its tile entity (null if none). Both are null if the chunk isn't loaded on a client.
		 *         On the server a missing chunk is generated first.
		 */
		std::pair<Tile *, TileEntity *> GetTileAt(sf::Vector2i position);

		/**
		 * @brief Changes the tile at a position.
		 *
		 * Does nothing if the tile and tile entity are already what is being set. A change made by a client is
		 * queued in `tilesToSend`. If a client's chunk isn't loaded yet, the change waits in `tileSetRequests`.
		 * @param position The tile's position in tiles.
		 * @param tile The new tile.
		 * @param tileEntity The tile's tile entity, or null if it has none.
		 * @param sentByServer Pass true when the change came from the server, so it isn't sent back.
		 */
		void SetTileAt(sf::Vector2i position, Tile tile, TileEntity *tileEntity = nullptr, bool sentByServer = false);

		/**
		 * @brief Writes or reads the planet's own saved data: the camera and the seed.
		 * @param s The serializer to use (its mode decides whether this reads or writes).
		 * @note Chunks, entities and the generator are saved separately by Save().
		 */
		void Serialize(Serializer &s);

		/**
		 * @brief Sets the seed for this planet and its generator. Do this before generating any chunks.
		 * @param seed The new seed.
		 */
		void SetSeed(uint64_t seed);

		/**
		 * @brief Generates a chunk and stores it in `chunks`.
		 * @param position The chunk's position in chunk coordinates.
		 * @warning Replaces the chunk if one already exists there, so check `chunks` first.
		 */
		void GenerateChunk(sf::Vector2i position);

		/**
		 * @brief Replaces an entity with a new one, for example when an update arrives from another player.
		 *
		 * The old entity is deleted. The new one is put into its chunk if that chunk is loaded.
		 * @param index The entity's index in `entities`.
		 * @param e The new entity. The planet takes ownership.
		 */
		void ReplaceEntity(int index, Entity *e);

		/**
		 * @brief Draws a picture on the planet using tiles.
		 *
		 * The image is scaled to the given width, and each pixel is replaced by the tile whose average colour
		 * is closest (compared in the CIELAB colour space), with dithering to approximate colours in between.
		 * Tile entities and fully transparent pixels are skipped. Does nothing if the image can't be loaded.
		 * @param imagePath The image file to load.
		 * @param placePos Where to put the image's top-left corner, in tiles.
		 * @param width How many tiles wide to make the picture. The height follows the image's shape.
		 */
		void MakeImageFromTiles(std::string &imagePath, sf::Vector2i placePos, int width);

		// A planet can't be copied (it holds unique_ptrs), but it can be moved, so it can be stored in a vector.
		Planet(const Planet &) = delete;
		Planet &operator=(const Planet &) = delete;
		Planet(Planet &&) = default;
		Planet &operator=(Planet &&) = default;
	};
}