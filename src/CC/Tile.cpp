#include "Tile.hpp"
#include "TileInfo.hpp"
#include "Planet.hpp"
#include "Item.hpp"
namespace cc
{
    void Tile::OnDestroy(Planet *p, sf::Vector2i chunkPos, sf::Vector2i subChunkPos)
    {
        static uint16_t treeId = TileInfo::nameLookup["Tree"]->id;
        static uint16_t rockId = TileInfo::nameLookup["Rock"]->id;
        if (type == treeId)
        {
            Item *i = new Item();
            i->position = (sf::Vector2f)(chunkPos * CHUNK_SIZE + subChunkPos);
            i->itemData.type = ItemData::WOOD;
            p->AddEntity(i, true, true);
        }
        else if (type == rockId)
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

            float random = dist(gen);
            Item *i = new Item();
            i->position = (sf::Vector2f)(chunkPos * CHUNK_SIZE + subChunkPos);
            if (random < 0.1f)
            {
                i->itemData.type = ItemData::FLINT;
            }else
            {
                i->itemData.type = ItemData::STONE;
            }
            p->AddEntity(i, true, true);
        }
    }
}