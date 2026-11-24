#include "ActionDestroyTile.hpp"
#include "Planet.hpp"
#include "Utils.hpp"
#include "Human.hpp"
#include "TileInfo.hpp"
namespace cc
{
    ActionDestroyTile::ActionDestroyTile()
    {
        type = ActionType::DESTROY_TILE;
    }
    void ActionDestroyTile::Tick()
    {
        static sf::Vector2i offsets[4]
        {
            {0,-1},
            {1,0},
            {0,1},
            {-1,0}
        };
        if (destroyTime == -1)
        {
            sf::Vector2i offset = offsets[direction];
            sf::Vector2i position = EntityToTilePos(context.human->position) + offset;
            auto tile = context.planet->GetTileAt(position);
            //if air just say done
            if (tile.first->type == 0)
            {
                finished = true;
                return;
            }
            destroyTime = TileInfo::tileRegistry[tile.first->type].destroyTime * 60;
        }
        progress ++;
        if (progress >= destroyTime)
        {
            sf::Vector2i offset = offsets[direction];
            sf::Vector2i position = EntityToTilePos(context.human->position) + offset;
            context.planet->DestroyTile(position);
            finished = true;
        }

    }
    void ActionDestroyTile::Serialize(Serializer& s)
    {
        Action::Serialize(s);
        s.field("direction",direction);
        s.field("progress",progress);
        s.field("destroyTime",destroyTime);
    }
}