#pragma once
#include "KnowledgeDatabase.hpp"
#include "Human.hpp"

namespace cc
{
    /**
     * @brief The rules for how humans gain knowledge, including how it spreads to related technologies.
     *
     * When a human gains understanding or skill in a technology, a share also goes to the technologies it
     * connects to, scaled by each connection's weight. The share shrinks with every step and stops once it is
     * below `minSpread`, so chains and cycles always end.
     *
     * Holds a reference to the database, so it must not outlive it. Meant to run on the server.
     */
    class KnowledgeSystem
    {
    public:
        explicit KnowledgeSystem(const KnowledgeDatabase &database);

        // ---- tuning ----
        /// A human counts as knowing a technology once their understanding reaches this. Below it, they only have
        /// the small amounts that spread from related technologies, so they can still discover it themselves.
        float knownUnderstanding = 0.25f;
        /// Fraction of a gain in understanding that passes along a connection (before the connection's weight).
        float understandingSpread = 0.5f;
        /// Same for skill. Lower than understanding, since skill mostly comes from doing the thing itself.
        float skillSpread = 0.25f;
        /// Spread amounts below this are dropped.
        float minSpread = 0.005f;
        /// How much existing understanding speeds up skill gain in Practice(). At 1, full understanding doubles it.
        float skillBoostFromUnderstanding = 1.f;

        // ---- gaining knowledge ----

        /// @brief Adds to a human's awareness of a technology (capped at 1). Does not spread.
        void GainAwareness(Human &human, int techId, float amount) const;

        /// @brief Adds to a human's understanding of a technology, and spreads a share to connected technologies.
        void GainUnderstanding(Human &human, int techId, float amount) const;

        /// @brief Adds to a human's skill at a technology, and spreads a share to connected technologies.
        void GainSkill(Human &human, int techId, float amount) const;

        /// @brief A human practising a technology: gains skill, faster the more they understand it.
        /// @param baseAmount Skill gained with no understanding.
        void Practice(Human &human, int techId, float baseAmount) const;

        /// @brief Whether a human knows a technology (understanding at or above `knownUnderstanding`).
        bool Knows(const Human &human, int techId) const;

        /**
         * @brief Whether a human can discover a technology right now: they don't know it yet and every
         *        requirement of its node is met.
         * @param currentAction The action the human is doing at this moment ("" if none). ACTION requirements
         *        are met only by this.
         */
        bool CanDiscover(const Human &human, int techId, const std::string &currentAction = "") const;

    private:
        enum class Stat
        {
            UNDERSTANDING,
            SKILL
        };

        /// @brief Adds `amount` to the stat of `techId` and passes shrinking shares along outgoing connections.
        ///        Each technology is reached at most once, by the strongest route to it.
        void Spread(Human &human, int techId, float amount, Stat stat, float spreadFactor) const;

        bool IsValidTech(int techId) const;

        bool MeetsRequirement(const Human &human, const UnlockRequirement &requirement,
                              const std::string &currentAction) const;

        /// Whether the human has at least `quantity` of the named item. Stub until the inventory exists.
        bool HasItem(const Human &human, const std::string &name, float quantity) const;

        const KnowledgeDatabase &database;
    };
}