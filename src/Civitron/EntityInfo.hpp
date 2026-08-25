#pragma once
#include "Atlas.hpp"
#include "../json.hpp"
namespace Civitron
{
    namespace EntityInfo
    {
        extern Atlas atlas;
        extern nlohmann::json texturesJson;
        void Init();
        void TransformJson(nlohmann::json &j);
        void Build();
    }
}