#include "Entity.hpp"
#include "Human.hpp"
#include "Item.hpp"
namespace cc
{
    Entity *CreateEntityFromType(Entity::EntityType type)
    {
        switch (type)
        {
        case Entity::NONE:
            return new Entity();
        case Entity::HUMAN:
            return new Human();
        case Entity::ITEM:
            return new Item();
        }
        return nullptr;
    }
}