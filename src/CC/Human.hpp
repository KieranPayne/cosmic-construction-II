#pragma once
#include "Entity.hpp"

namespace cc
{
    /**
     * @brief A human character in the world.
     */
    class Human : public Entity
    {
    public:
        int movementCounter = 0;
        /// @brief Creates a human entity.
        Human();

        /**
         * @brief Gets where this entity's sprite is in the entity texture atlas.
         * @return The sprite's top-left corner, in pixels.
         */
        sf::Vector2f GetTexCoords();

        void Tick();
        void Serialize(Serializer& s);
    };
}