#pragma once
#include "../json.hpp"
namespace Civitron{
    namespace ItemInfo{
        //each item id will have a texture and name
        extern nlohmann::json itemsJson;
        void Init();
    }
}