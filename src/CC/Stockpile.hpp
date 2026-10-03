#pragma once
#include "TileEntity.hpp"
#include "TileInfo.hpp"
namespace cc
{
    /**
     * @brief A tile entity for the "Stockpile" tile.
     */
    class Stockpile : public TileEntity
    {
    public:
        /// @brief Creates a stockpile. Sets the tile type to the id registered for "Stockpile" (looked up once, then reused).
        Stockpile()
        {
            static int myType = TileInfo::nameLookup["Stockpile"]->id;
            type = myType;
        }

        /**
         * @brief Adds the vertices for this tile to a chunk's vertex array.
         *
         * Writes one quad (two triangles, six vertices) at the tile's world position, textured with the
         * "Stockpile" tile's sprite.
         * @param arr The vertex array to write into. Must have room for six more vertices at `index`.
         * @param index Where in `arr` to start writing. Increased by 6 for the vertices written.
         */
        void GetVertices(sf::VertexArray &arr, int &index);
    };
}