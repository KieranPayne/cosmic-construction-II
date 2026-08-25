#include "Item.hpp"
#include "Utils.hpp"
#include "ItemInfo.hpp"
#include "Planet.hpp"
#include "TileInfo.hpp"
namespace Civitron{
    Item::Item(){
        position = {0,0,0};
        myClass = ITEM;
        data = {0,1};
    }
    sf::Vector2f Item::GetTexCoords(int height){
        nlohmann::json j = ItemInfo::itemsJson[std::to_string(data.id)];
        return JsonAsVector(j["coordinates"]);
    }

    void Item::Tick(Planet* planet){
        if (planet->GetTileAt(position - sf::Vector3i(0,1,0))->type == GetTileID("Air")){
            MoveTo(position - sf::Vector3i(0,1,0));
        }
        sf::Vector3i chunkPos = TileToChunkPos(position);
        auto& entities = planet->chunks[chunkPos]->entities;
        for (int i = 0; i < entities.size(); i ++){
            auto e = entities[i];
            if (e != this && e->myClass == ITEM && e->position == position){
                Item* i = (Item*)(e);
                if (i->data.Equals(&data)){
                    data.amount += i->data.amount;
                    planet->RemoveEntity(e);
                }
            }
        }
    }

    void Item::FromJson(nlohmann::json& j){
        position = {j["position"][0],j["position"][1],j["position"][2]};
        data.FromJson(j["data"]);
    }
    nlohmann::json Item::ToJson(){
        nlohmann::json j = Entity::ToJson();
        j["position"] = {position.x,position.y,position.z};
        j["data"] = data.ToJson();
        return j;
    }
}