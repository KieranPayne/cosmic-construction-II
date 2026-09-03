#pragma once
#include "../json.hpp"
namespace cc
{
    class TileEntity
    {
        public:
        uint16_t type = 0;
        TileEntity(){}
        nlohmann::json ToJson()
        {
            nlohmann::json j;
            j["type"] = type;
            return j;
        }
        void FromJson(nlohmann::json& j)
        {
            type = j["type"];
        }
    };
    TileEntity* CreateTileEntityFromType(uint16_t type);
}