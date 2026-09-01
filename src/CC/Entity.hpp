#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "EntityInfo.hpp"
#include "Utils.hpp"
#include "Tile.hpp"
namespace cc
{

    class Entity
    {
        public:
        enum EntityType
        {
            NONE = 0,
            HUMAN,
            ITEM
        };
        Entity()
        {
        };
        EntityType type = NONE;
        sf::Vector2i chunkPos;
        sf::Vector2f position = {0.f,0.f};
        sf::Vector2f size = {1.f,1.f};
        virtual std::vector<sf::Vertex> GetVerts()
        {
            sf::Vector2f offsets[6] = {
                {0, 0},
                {TILE_SIZE * size.x, 0},
                {TILE_SIZE * size.x, TILE_SIZE * size.y},
                {0, 0},
                {TILE_SIZE * size.x, TILE_SIZE * size.y},
                {0, TILE_SIZE * size.y}
            };

            sf::Vector2f texCoords = GetTexCoords();
            std::vector<sf::Vertex> verts;

            for (int i = 0; i < 6; i++)
            {
                sf::Vertex v;

                v.position = position * (float)TILE_SIZE + offsets[i];
                v.texCoords = texCoords + offsets[i];

                verts.push_back(v);
            }

            return verts;
        }
        virtual sf::Vector2f GetTexCoords()
        {
            return JsonAsVector(EntityInfo::texturesJson["Entity"]);
        }
        virtual void Tick()
        {
            // position += {0.1f,0.f};
        };
        virtual nlohmann::json ToJson()
        {
            nlohmann::json j;
            j["type"] = (int)type;
            j["position"] = {position.x,position.y};
            j["size"] = {size.x,size.y};
            return j;
        }
        virtual void FromJson(nlohmann::json& j)
        {
            type = (EntityType)(j["type"]);
            position = {j["position"][0],j["position"][1]};
            size = {j["size"][0],j["size"][1]};
        }
    };
}
