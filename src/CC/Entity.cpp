#include "Entity.hpp"
#include "Human.hpp"
#include "Item.hpp"
#include "Planet.hpp"
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
    void Entity::FixPointers(Planet* planet)
    {
        world = planet->world;
        this->planet = planet;
    }
}