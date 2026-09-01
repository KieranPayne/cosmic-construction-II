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
            if (j.is_string())
            {
                std::string val = j.get<std::string>();

                sf::Texture tex;

                if (!tex.loadFromFile("content/resources/" + val))
                {
                    std::cout << "failed to load entity texture: "
                              << val << std::endl;
                    return;
                }

                atlas.AddTexture(tex);

                j = {
                    atlas.positions.back().x,
                    atlas.positions.back().y};

                return;
            }

            if (j.is_object())
            {
                for (auto &[key, value] : j.items())
                {
                    TransformJson(value);
                }
            }
            else if (j.is_array())
            {
                for (auto &value : j)
                {
                    TransformJson(value);
                }
            }
        }
    }
}