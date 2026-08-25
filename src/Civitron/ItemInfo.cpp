#include "ItemInfo.hpp"
#include "../PCH.hpp"
#include "EntityInfo.hpp"
namespace Civitron{
    namespace ItemInfo{
        nlohmann::json itemsJson;
        void Init(){
            std::ifstream f("content/resources/ItemInfo.json");
            itemsJson = nlohmann::json::parse(f);
            f.close();
            for (auto& j : itemsJson){
                sf::Texture tex;   
                int pos[2];
                std::string val = j["path"];
                if (!tex.loadFromFile("content/resources/" + val))
                {
                    std::cout << "failed to load item texture" << std::endl;
                }
                EntityInfo::atlas.AddTexture(tex);
                pos[0] = EntityInfo::atlas.positions.back().x;
                pos[1] = EntityInfo::atlas.positions.back().y;
                j["coordinates"] = {pos[0],pos[1]};
            }
        }
    }
}