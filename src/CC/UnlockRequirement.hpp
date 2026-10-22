#pragma once
#include <string>
#include "Serializer.hpp"

namespace cc
{
    /**
     * @brief One factor that must be true before a human can discover a technology.
     *
     * A technology lists several of these and needs all of them at once. Each instance is a single
     * factor, so combining them (items + a prerequisite concept, for example) is just a list.
     */
    class UnlockRequirement
    {
    public:
        enum class Type : int
        {
            /// The human has `amount` of the item called `name` to hand (stick, hide, stone...).
            ITEM = 0,
            /// The human has at least `amount` (0 to 1) understanding of the graph node `node`.
            CONCEPT,
            /// The human has recently been doing the action called `name` (striking, tending a fire...).
            ACTION
        };

        Type type = Type::ITEM;
        /// Item or action name. Used by ITEM and ACTION.
        std::string name = "";
        /// Index of a node in the graph. Used by CONCEPT. -1 means none chosen.
        int node = -1;
        /// ITEM: how many are needed. CONCEPT: minimum understanding, 0 to 1. ACTION: how much recent activity counts.
        float amount = 1.f;

        void Serialize(Serializer &s)
        {
            // The serializer has no enum overload, so the type is stored as an int.
            int t = static_cast<int>(type);
            s.field("type", t);
            type = static_cast<Type>(t);

            s.field("name", name);
            s.field("node", node);
            s.field("amount", amount);
        }
    };
}