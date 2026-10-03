#include "TileEntity.hpp"
#include "TileInfo.hpp"
#include "Stockpile.hpp"
#include "Tile.hpp"
#include "Utils.hpp"
#include "Chunk.hpp"
namespace cc
{
    TileEntity *CreateTileEntityFromType(uint16_t type)
    {
        static int stockpileType = TileInfo::nameLookup["Stockpile"]->id;
        if (type == 0)
        {
            return new TileEntity();
        }
        else if (type == stockpileType)
        {
            return new Stockpile();
        }
        return nullptr;
    }
    void TileEntity::GetVertices(sf::VertexArray &arr, int &index)
    {
        static const sf::Vector2f offsets[6] = {
            {0, 0},
            {TILE_SIZE, 0},
            {TILE_SIZE, TILE_SIZE},
            {0, 0},
            {TILE_SIZE, TILE_SIZE},
            {0, TILE_SIZE}};
        sf::Vector2f texCoords = (sf::Vector2f)TileInfo::tileRegistry[type].positions[0];
        for (int i = 0; i < 6; i++)
        {
            sf::Vertex v;
            v.position = (sf::Vector2f)(position * TILE_SIZE + chunk->position * TILE_SIZE * CHUNK_SIZE) + offsets[i];
            v.texCoords = texCoords + offsets[i];
            arr[index] = v;
            index++;
        }
    }
}