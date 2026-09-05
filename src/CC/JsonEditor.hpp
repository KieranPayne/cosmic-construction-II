
#pragma once
#include "../imgui/imgui.h"
#include "../json.hpp"

#include <string>
#include <unordered_map>
#include <cstdint>
#include "Entity.hpp"
namespace cc
{
    class Planet;
    class JsonEditor
    {
    public:
        void Draw(Planet *p);
        
        bool DrawEntity(nlohmann::json &value, const char *label)
        {
            return DrawValue(value, label, label);
        }

    private:
        bool addingEntity = false;
        Entity* entity = nullptr;
        std::unordered_map<std::string, std::string> stringBuffers;
        bool DrawValue(
            nlohmann::json &value,
            const char *label,
            const std::string &path);
    };
}