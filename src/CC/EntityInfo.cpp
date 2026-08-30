#include "EntityInfo.hpp"
#include <iostream>
#include <fstream>
namespace cc
{
    namespace EntityInfo
    {
        Atlas atlas;
        nlohmann::json texturesJson;
        void Init()
        {
            std::ifstream f("content/resources/EntityTextures.json");
            texturesJson = nlohmann::json::parse(f);
            f.close();
            TransformJson(texturesJson);
        }
        void Build()
        {
            atlas.Build();
        }
        void TransformJson(nlohmann::json &j)
        {
            if (j.is_object())
            {
                for (auto &[key, value] : j.items())
                {
                    if (value.is_string())
                    {
                        std::string val = value.get<std::string>();
                        sf::Texture tex;
                        int pos[2];
                        if (!tex.loadFromFile("content/resources/" + val))
                        {
                            std::cout << "failed to load entity texture" << std::endl;
                        }
                        atlas.AddTexture(tex);
                        pos[0] = atlas.positions.back().x;
                        pos[1] = atlas.positions.back().y;
                        // Replace string with pos array
                        value = {pos[0], pos[1]};
                    }
                    else if (value.is_object())
                    {
                        // Recurse into sub-objects
                        TransformJson(value);
                    }
                }
            }
        }
    }
}