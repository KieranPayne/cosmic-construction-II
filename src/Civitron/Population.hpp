#pragma once
#include "../PCH.hpp"
#include "Human.hpp"
namespace Civitron{
    class Population{
        public:
        Population();
        Planet* planet;
        std::vector<Human*> humans;
        nlohmann::json ToJson();
        void FromJson(nlohmann::json& j);
    };
}