#pragma once
#include "Entity.hpp"

namespace cc
{
    class Human : public Entity
    {
        public:
        Human();
        sf::Vector2f GetTexCoords();
    };
}