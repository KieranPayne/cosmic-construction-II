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
        static uint16_t tallGrassId = TileInfo::nameLookup["Tall grass"]->id;

        if (type == treeId)
        {
            Item *i = new Item();
            i->position = (sf::Vector2f)(chunkPos * CHUNK_SIZE + subChunkPos);
            i->itemData.type = ItemData::WOOD;
            p->AddEntity(i);
        }
        else if (type == rockId)
        {
            uint64_t roll = p->rng() % 1000;
            Item *i = new Item();
            i->position = (sf::Vector2f)(chunkPos * CHUNK_SIZE + subChunkPos);
            if (roll < 100) // 10% chance
            {
                i->itemData.type = ItemData::FLINT;
            }
            else
            {
                i->itemData.type = ItemData::STONE;
            }
            p->AddEntity(i);
        }
        else if (type == tallGrassId)
        {
            uint64_t roll = p->rng() % 1000;
            if (roll < 300) // 30% chance
            {
                Item *i = new Item();
                i->position = (sf::Vector2f)(chunkPos * CHUNK_SIZE + subChunkPos);
                i->itemData.type = ItemData::FIBER;
                p->AddEntity(i);
            }
        }
    }
}