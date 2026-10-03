#pragma once
#include "../json.hpp"
#include "../PCH.hpp"
#include "Serializer.hpp"
namespace cc
{
    class Chunk;

    /**
     * @brief Extra data and behaviour attached to a single tile, for tiles that need more than a type.
     *
     * The base class is used for tile type 0. Specific kinds (for example Stockpile) derive from it.
     */
    class TileEntity
    {
    public:
        /// The tile type this tile entity belongs to (an index into TileInfo::tileRegistry).
        uint16_t type = 0;

        /// Where the tile is within its chunk, in tiles.
        sf::Vector2i position;

        /// The chunk this tile entity is in. Used to work out its world position. Has no initial value.
        Chunk *chunk;

        TileEntity() {}

        /**
         * @brief Writes or reads the tile entity's type and position.
         * @param s The serializer to use (its mode decides whether this reads or writes).
         */
        void Serialize(Serializer &s)
        {
            s.field("type", type);
            s.field("position", position);
        }

        /**
         * @brief Adds the vertices for this tile to a chunk's vertex array.
         *
         * Writes one quad (two triangles, six vertices) at the tile's world position, using the sprite of
         * its tile type. Derived classes can override this to draw differently.
         * @param arr The vertex array to write into. Must have room for six more vertices at `index`.
         * @param index Where in `arr` to start writing. Increased by 6 for the vertices written.
         */
        virtual void GetVertices(sf::VertexArray &arr, int &index);
    };

    /**
     * @brief Creates the right kind of tile entity for a tile type.
     * @param type The tile type.
     * @return A new tile entity (the caller owns it): a Stockpile for the stockpile type, a plain TileEntity for
     *         type 0, or nullptr for any other type.
     */
    TileEntity *CreateTileEntityFromType(uint16_t type);
}