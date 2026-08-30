#pragma once
#include "../json.hpp"
#include "../PCH.hpp"
#include "Atlas.hpp"
namespace cc
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