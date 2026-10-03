#pragma once
#include "../PCH.hpp"
#include "Tile.hpp"
#include "Entity.hpp"
#include "TileEntity.hpp"
namespace cc
{
/// Width and height of a chunk, in tiles.
#define CHUNK_SIZE 32

	/// Number of bytes the per-tile section of GetByteData() takes up: for each tile, 2 bytes of tile type
	/// plus 4 bytes of background data (red, green, blue, background type). Does not include the tile entity section.
	constexpr int CHUNK_NUM_BYTES = CHUNK_SIZE * CHUNK_SIZE * (2 + 4);

	/**
	 * @brief A CHUNK_SIZE x CHUNK_SIZE square of the world: its tiles, background tiles,
	 *        tile entities, and the entities currently standing in it.
	 *
	 * Tile coordinates passed to or returned from Chunk functions are local to the chunk, from
	 * (0, 0) to (CHUNK_SIZE - 1, CHUNK_SIZE - 1), not world tile coordinates.
	 */
	class Chunk
	{
	public:
		/// Foreground tiles, indexed [x][y] by local tile coordinates. Type 0 is air.
		Tile tiles[CHUNK_SIZE][CHUNK_SIZE] = {};

		/// Background (terrain) tiles, indexed [x][y] by local tile coordinates.
		BackgroundTile backgroundTiles[CHUNK_SIZE][CHUNK_SIZE] = {};

		/// Position of this chunk in chunk coordinates (not tiles or pixels).
		sf::Vector2i position;

		/// Entities currently in this chunk. These are not owned by the chunk (the planet owns them),
		/// and the chunk never deletes them.
		std::vector<Entity *> entities = {};

		/// Tile entities (tiles with extra data or behaviour), owned by the chunk. The key is the tile's
		/// position in the chunk as given by TileEntityIndex().
		std::unordered_map<uint16_t, std::unique_ptr<TileEntity>> tileEntities;

		/**
		 * @brief Creates an empty chunk: every tile is air and every background colour is white.
		 * @param position The chunk's position in chunk coordinates.
		 */
		Chunk(sf::Vector2i position);

		/// @brief Creates a chunk at chunk position (0, 0).
		Chunk();

		/**
		 * @brief Serializes the chunk's tiles, backgrounds and tile entities into bytes.
		 *
		 * Layout:
		 *  1. A uint32 (native byte order) giving the size of the tile entity block.
		 *  2. The tile entity block: a count, then for each tile entity its type, key and data.
		 *  3. For each tile, with x as the outer loop and y as the inner loop: the tile type as two
		 *     bytes (high byte first), then the background colour's red, green and blue, then the
		 *     background tile type, one byte each.
		 *
		 * This is the format sent to clients and written to disk.
		 * @return The serialized chunk.
		 */
		std::vector<uint8_t> GetByteData();

		/**
		 * @brief Loads the chunk's contents from bytes produced by GetByteData().
		 * @param bytes The serialized chunk. It must be complete and well formed: sizes are not
		 *              checked, so truncated or corrupt data is undefined behaviour.
		 * @note Tile entities in the data are added to `tileEntities`, replacing any with the same key.
		 *       Existing ones with other keys are not removed.
		 */
		void LoadByteData(std::vector<uint8_t> &bytes);

		/**
		 * @brief Writes the chunk to a file as a uint32 length followed by the bytes from GetByteData().
		 * @param path The file to create or overwrite.
		 */
		void WriteData(std::string path);

		/**
		 * @brief Reads a file written by WriteData() and loads it into this chunk.
		 * @param path The file to read.
		 * @note Does not check that the file exists or is valid.
		 */
		void ReadData(std::string path);

		/**
		 * @brief Draws every entity in this chunk using the entity atlas, in a single draw call.
		 * @param target The render target to draw to. The current view of the target is used.
		 */
		void RenderEntities(sf::RenderTarget *target);

		/**
		 * @brief Registers an entity as being in this chunk.
		 * @param entity The entity. Not owned by the chunk; it is not checked for duplicates.
		 */
		void AddEntity(Entity *entity);

		/**
		 * @brief Unregisters an entity from this chunk. Does nothing if it is not in the chunk.
		 * @param entity The entity to remove. It is not deleted.
		 */
		void RemoveEntity(Entity *entity);

		/**
		 * @brief Unregisters the entity at a position in `entities`.
		 * @param index Index into `entities`. Not bounds checked. The entity is not deleted.
		 */
		void RemoveEntity(int index);

		/**
		 * @brief Replaces the tile at a position.
		 *
		 * Any tile entity already at that position is removed. If the new tile type is a tile entity
		 * type, a tile entity is stored for it, and its `position` and `chunk` are set.
		 * @param pos Local tile coordinates within the chunk.
		 * @param tile The new tile.
		 * @param tileEntity Data for the tile entity of the new tile, or nullptr to create a default one.
		 *                   The chunk takes ownership. Only pass a non-null value when `tile` is a tile
		 *                   entity type; otherwise it is neither stored nor deleted, so it would leak.
		 */
		void SetTile(sf::Vector2i pos, Tile tile, TileEntity *tileEntity = nullptr);

		/**
		 * @brief Gets the tile at a position and its tile entity, if it has one.
		 * @param pos Local tile coordinates within the chunk.
		 * @return A pair of the tile and its tile entity. The tile entity is nullptr if the tile is not a
		 *         tile entity type. Both pointers point into this chunk and stop being valid when it changes or is destroyed.
		 * @note If the tile is a tile entity type but none is stored, a null entry is added to `tileEntities`.
		 */
		std::pair<Tile *, TileEntity *> GetTile(sf::Vector2i pos);

		/**
		 * @brief Converts a position in the chunk to the key used in `tileEntities`.
		 * @param pos Local tile coordinates within the chunk.
		 * @return `pos.y * CHUNK_SIZE + pos.x`.
		 */
		uint16_t TileEntityIndex(sf::Vector2i pos);

		/**
		 * @brief Deletes the tile entity with the given key. Does nothing if there is none.
		 * @param index Key as returned by TileEntityIndex().
		 * @note The tile itself in `tiles` is not changed.
		 */
		void RemoveTileEntity(uint16_t index);
	};

	/**
	 * @brief Hash function for chunk positions, for use as the hash of an unordered_map or
	 *        unordered_set keyed by sf::Vector2i.
	 */
	struct ChunkHash
	{
		/// @brief Returns a hash combining the x and y of a position.
		std::size_t operator()(const sf::Vector2i &pos) const
		{
			std::uint64_t x = static_cast<std::uint32_t>(pos.x);
			std::uint64_t y = static_cast<std::uint32_t>(pos.y);

			std::uint64_t h = x * 0x9E3779B185EBCA87ULL;
			h ^= y + 0x9E3779B185EBCA87ULL + (h << 6) + (h >> 2);

			return static_cast<std::size_t>(h);
		}
	};

}