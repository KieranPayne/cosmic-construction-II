#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "Serializer.hpp"
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
        void Serialize(Serializer& s)
        {
            uint16_t t = (uint16_t)type;
            s.field("type",t);
            type = (ItemType)t;
            s.field("amount",amount);
        }
        // nlohmann::json ToJson()
        // {
        //     nlohmann::json j;
        //     j["type"] = (uint16_t)type;
        //     j["amount"] = amount;
        //     return j;
        // }
        // void FromJson(nlohmann::json& j)
        // {
        //     type = (ItemType)j["type"];
        //     amount = j["amount"];
        // }
    };
}