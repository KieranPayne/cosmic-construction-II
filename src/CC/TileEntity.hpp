#pragma once
#include "../json.hpp"
#include "../PCH.hpp"
namespace cc
{
    class Chunk;
    class TileEntity
    {
        public:
        uint16_t type = 0;
        sf::Vector2i position;
        Chunk* chunk;
        TileEntity(){}
        nlohmann::json ToJson()
        {
            nlohmann::json j;
            j["type"] = type;
            j["position"] = {position.x,position.y};
            return j;
        }
        void FromJson(nlohmann::json& j)
        {
            type = j["type"];
            position = {j["position"][0],j["position"][1]};
        }
        virtual void GetVertices(sf::VertexArray& arr, int& index);
    };
    TileEntity* CreateTileEntityFromType(uint16_t type);
}