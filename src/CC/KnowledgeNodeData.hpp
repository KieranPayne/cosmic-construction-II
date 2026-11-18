#pragma once
#include "../PCH.hpp"
#include "Serializer.hpp"
#include "UnlockRequirement.hpp"

namespace cc
{
    class KnowledgeNodeData
    {
    private:
        std::string name = "";
        std::string description = "";
        sf::Color color = sf::Color(235, 64, 52);

    public:
        sf::Color GetColor() const;
        /// Everything that must be true at once for a human to discover this technology.
        std::vector<UnlockRequirement> requirements;
        // sf::Color color = sf::Color(226,121,16);
        const std::string &GetName() const { return name; }
        KnowledgeNodeData();
        void Serialize(Serializer &s);
    };
}