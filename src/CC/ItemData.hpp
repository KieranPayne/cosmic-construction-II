#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
namespace cc
{
    class ItemData
    {
        public:
        enum ItemType : uint16_t
        {
            TEST1 = 0,
            TEST2
        };
        ItemType type = TEST1;
        unsigned int amount = 1;
        nlohmann::json ToJson()
        {
            nlohmann::json j;
            j["type"] = (uint16_t)type;
            j["amount"] = amount;
            return j;
        }
        void FromJson(nlohmann::json& j)
        {
            type = (ItemType)j["type"];
            amount = j["amount"];
        }
    };
}