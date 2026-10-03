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
        /// @brief Creates a human entity.
        Human();

        /**
         * @brief Gets where this entity's sprite is in the entity texture atlas.
         * @return The sprite's top-left corner, in pixels.
         */
        sf::Vector2f GetTexCoords();
    };
}