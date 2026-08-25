#include "ItemData.hpp"

namespace Civitron{
    bool ItemData::Equals(ItemData* other){
        return other->id == id;
    }
    void ItemData::FromJson(nlohmann::json& j){
        id = j["id"];
        amount = j["amount"];
    }
    nlohmann::json ItemData::ToJson(){
        nlohmann::json j;
        j["id"] = id;
        j["amount"] = amount;
        return j;
    }
}