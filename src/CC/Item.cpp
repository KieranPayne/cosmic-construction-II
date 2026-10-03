#include "Item.hpp"

namespace cc
{
    Item::Item()
    {
        type = ITEM;
    }
    sf::Vector2f Item::GetTexCoords()
    {
        return JsonAsVector(EntityInfo::texturesJson["Item"][(uint16_t)itemData.type]);
    }
    void Item::Serialize(Serializer &s)
    {
        Entity::Serialize(s);
        s.field("itemData", itemData);
    }
}