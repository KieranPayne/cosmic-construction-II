#include "KnowledgeNodeData.hpp"

namespace cc
{
    KnowledgeNodeData::KnowledgeNodeData()
    {
    }
    void KnowledgeNodeData::Serialize(Serializer &s)
    {
        s.field("name", name);
        s.field("description", description);
        s.field("color", color);
        s.field("requirements", requirements);
    }
    sf::Color KnowledgeNodeData::GetColor() const
    {
        return color;
    } 
}