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
    nlohmann::json Item::ToJson()
    {
        nlohmann::json j = Entity::ToJson();
        j["itemData"] = itemData.ToJson();
        return j;
    }
    void Item::FromJson(nlohmann::json& j)
    {
        Entity::FromJson(j);
        itemData.FromJson(j["itemData"]);
    }
}