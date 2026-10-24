#include "KnowledgeSystem.hpp"

#include <algorithm>
#include <queue>

namespace cc
{
    KnowledgeSystem::KnowledgeSystem(const KnowledgeDatabase &database)
        : database(database)
    {
    }

    bool KnowledgeSystem::IsValidTech(int techId) const
    {
        return techId >= 0 && techId < database.NodeCount();
    }

    void KnowledgeSystem::GainAwareness(Human &human, int techId, float amount) const
    {
        if (!IsValidTech(techId))
        {
            return;
        }
        HumanKnowledge &k = human.GetOrCreateKnowledge(techId);
        k.awareness = std::min(1.f, k.awareness + amount);
    }

    void KnowledgeSystem::GainUnderstanding(Human &human, int techId, float amount) const
    {
        Spread(human, techId, amount, Stat::UNDERSTANDING, understandingSpread);
    }

    void KnowledgeSystem::GainSkill(Human &human, int techId, float amount) const
    {
        Spread(human, techId, amount, Stat::SKILL, skillSpread);
    }

    void KnowledgeSystem::Practice(Human &human, int techId, float baseAmount) const
    {
        float boost = 1.f + skillBoostFromUnderstanding * human.GetUnderstanding(techId);
        GainSkill(human, techId, baseAmount * boost);
    }

    void KnowledgeSystem::Spread(Human &human, int techId, float amount, Stat stat, float spreadFactor) const
    {
        if (!IsValidTech(techId) || amount <= 0.f)
        {
            return;
        }

        struct Pending
        {
            float amount;
            int node;
            // Largest amount first, so each technology is reached by its strongest route.
            bool operator<(const Pending &other) const { return amount < other.amount; }
        };

        std::priority_queue<Pending> queue;
        std::vector<bool> done(database.NodeCount(), false);
        queue.push({amount, techId});

        while (!queue.empty())
        {
            Pending current = queue.top();
            queue.pop();

            if (done[current.node])
            {
                continue;
            }
            done[current.node] = true;

            HumanKnowledge &k = human.GetOrCreateKnowledge(current.node);
            float &value = (stat == Stat::UNDERSTANDING) ? k.understanding : k.skill;
            value = std::min(1.f, value + current.amount);

            for (int connectionIndex : database.OutgoingConnections(current.node))
            {
                const KnowledgeConnection &c = database.GetConnection(connectionIndex);
                if (!IsValidTech(c.end) || done[c.end])
                {
                    continue;
                }

                float next = current.amount * c.weight * spreadFactor;
                if (next >= minSpread)
                {
                    queue.push({next, c.end});
                }
            }
        }
    }

    bool KnowledgeSystem::Knows(const Human &human, int techId) const
    {
        return human.GetUnderstanding(techId) >= knownUnderstanding;
    }

    bool KnowledgeSystem::CanDiscover(const Human &human, int techId, const std::string &currentAction) const
    {
        if (!IsValidTech(techId) || Knows(human, techId))
        {
            return false;
        }

        // Every requirement must hold at once.
        for (const UnlockRequirement &requirement : database.GetNode(techId).requirements)
        {
            if (!MeetsRequirement(human, requirement, currentAction))
            {
                return false;
            }
        }
        return true;
    }

    bool KnowledgeSystem::MeetsRequirement(const Human &human, const UnlockRequirement &requirement,
                                           const std::string &currentAction) const
    {
        switch (requirement.type)
        {
        case UnlockRequirement::Type::ITEM:
            return HasItem(human, requirement.name, requirement.amount);

        case UnlockRequirement::Type::CONCEPT:
            return IsValidTech(requirement.node) &&
                   human.GetUnderstanding(requirement.node) >= requirement.amount;

        case UnlockRequirement::Type::ACTION:
            return !currentAction.empty() && requirement.name == currentAction;
        }
        return false;
    }

    bool KnowledgeSystem::HasItem(const Human &human, const std::string &name, float quantity) const
    {
        // TODO: once humans have an inventory, count the items whose data has this name and compare to `quantity`.
        return false;
    }
}