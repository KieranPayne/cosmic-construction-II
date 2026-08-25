#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
namespace Civitron{
    struct ItemData{
        uint64_t id;
        uint16_t amount;
        bool Equals(ItemData* other);
        void FromJson(nlohmann::json& j);
        nlohmann::json ToJson();
    };
}