#include "Entity.hpp"
#include "Human.hpp"
#include "Item.hpp"
namespace cc{
    Entity* CreateEntityFromType(Entity::EntityType type)
    {
        if (type == Entity::NONE)
        {
            return new Entity();
        }else if (type == Entity::HUMAN)
        {
            return new Human();
        }else if (type == Entity::ITEM)
        {
            return new Item();
        }
        return nullptr;
    }
}