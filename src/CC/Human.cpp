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

    void Human::Tick()
    {
        movementCounter ++;
        if (movementCounter == 60)
        {
            movementCounter = 0;
            position.x ++;
        }
    }

    void Human::Serialize(Serializer& s)
    {
        Entity::Serialize(s);
        s.field("movementCounter",movementCounter);
    }
}