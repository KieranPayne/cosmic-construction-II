#include "Human.hpp"
namespace cc
{
    Human::Human()
    {
        type = HUMAN;
    }

    sf::Vector2f Human::GetTexCoords()
    {
        return JsonAsVector(EntityInfo::texturesJson["Human"]);
    }
}