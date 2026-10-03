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
            std::ifstream file("content/resources/EntityTextures.json");
            texturesJson = nlohmann::json::parse(file);
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
                // a texture file name: load it, add it to the atlas, and replace the name with its position
                std::string fileName = j.get<std::string>();

                sf::Texture texture;
                if (!texture.loadFromFile("content/resources/" + fileName))
                {
                    std::cout << "failed to load entity texture: " << fileName << '\n';
                    return;
                }

                atlas.AddTexture(texture);
                j = {atlas.positions.back().x, atlas.positions.back().y};
            }
            else if (j.is_object() || j.is_array())
            {
                // iterating an object visits its values, so this handles both
                for (auto &value : j)
                {
                    TransformJson(value);
                }
            }
        }
    }
}