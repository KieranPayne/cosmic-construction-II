#pragma once
#include "Entity.hpp"
#include "HumanKnowledge.hpp"

namespace cc
{
    /**
     * @brief A human character in the world.
     */
    class Human : public Entity
    {
    public:
        /// @brief Creates a human entity.
        Human();

        /**
         * @brief Gets where this entity's sprite is in the entity texture atlas.
         * @return The sprite's top-left corner, in pixels.
         */
        sf::Vector2f GetTexCoords();

        /// @brief Saves or loads the entity's data plus this human's knowledge.
        void Serialize(Serializer &s);

        /// Everything this human knows about technologies, one entry per technology they have any knowledge of.
        std::vector<HumanKnowledge> knowledge;

        /// @brief Finds this human's knowledge of a technology.
        /// @param techId Index of the technology in the knowledge database.
        /// @return The entry, or nullptr if they have none.
        HumanKnowledge *FindKnowledge(int techId);
        const HumanKnowledge *FindKnowledge(int techId) const;

        /// @brief Finds this human's knowledge of a technology, adding an empty entry if they have none.
        HumanKnowledge &GetOrCreateKnowledge(int techId);

        /// @brief How well this human understands a technology, from 0 to 1. 0 if they have no entry.
        float GetUnderstanding(int techId) const;

        /// @brief How skilled this human is at a technology, from 0 to 1. 0 if they have no entry.
        float GetSkill(int techId) const;
    };
}