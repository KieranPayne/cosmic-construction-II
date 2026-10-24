#include "Human.hpp"
namespace cc
{
    Human::Human()
    {
        type = HUMAN;
    }

    sf::Vector2f Human::GetTexCoords()
    {
        return JsonAsVector(EntityInfo::texturesJson["Human"]);
    }

    void Human::Serialize(Serializer &s)
    {
        Entity::Serialize(s);
        s.field("knowledge", knowledge);
    }

    HumanKnowledge *Human::FindKnowledge(int techId)
    {
        for (auto &k : knowledge)
        {
            if (k.techId == techId)
            {
                return &k;
            }
        }
        return nullptr;
    }

    const HumanKnowledge *Human::FindKnowledge(int techId) const
    {
        for (auto &k : knowledge)
        {
            if (k.techId == techId)
            {
                return &k;
            }
        }
        return nullptr;
    }

    HumanKnowledge &Human::GetOrCreateKnowledge(int techId)
    {
        if (HumanKnowledge *existing = FindKnowledge(techId))
        {
            return *existing;
        }
        HumanKnowledge k;
        k.techId = techId;
        knowledge.push_back(k);
        return knowledge.back();
    }

    float Human::GetUnderstanding(int techId) const
    {
        const HumanKnowledge *k = FindKnowledge(techId);
        return k ? k->understanding : 0.f;
    }

    float Human::GetSkill(int techId) const
    {
        const HumanKnowledge *k = FindKnowledge(techId);
        return k ? k->skill : 0.f;
    }
}