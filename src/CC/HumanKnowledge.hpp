#pragma once
#include "Serializer.hpp"

namespace cc
{
    /**
     * @brief What one human knows about one technology.
     *
     * All three values run from 0 to 1.
     */
    struct HumanKnowledge
    {
        /// Index of the technology's node in the KnowledgeDatabase.
        int techId = -1;
        /// Knows the technology exists (for example, from watching someone use it).
        float awareness = 0.f;
        /// Knows how it is made. Raised by teaching, reading, and working it out.
        float understanding = 0.f;
        /// How well they can actually make it. Raised by practising.
        float skill = 0.f;

        void Serialize(Serializer &s)
        {
            s.field("techId", techId);
            s.field("awareness", awareness);
            s.field("understanding", understanding);
            s.field("skill", skill);
        }
    };
}