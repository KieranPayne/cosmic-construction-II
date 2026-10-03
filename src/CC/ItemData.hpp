#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "Serializer.hpp"
namespace cc
{
    /**
     * @brief What an item is and how many of it there are.
     *
     * Used by the Item entity, and anywhere else that needs to hold a stack of items.
     */
    class ItemData
    {
    public:
        /// The kinds of item. Stored as a uint16_t. TEST1 and TEST2 are placeholders.
        enum ItemType : uint16_t
        {
            TEST1 = 0,
            TEST2
        };

        /// What kind of item this is. Also picks which sprite an Item entity draws.
        ItemType type = TEST1;

        /// How many of the item there are (the size of the stack).
        unsigned int amount = 1;

        /**
         * @brief Writes or reads the item type and amount.
         * @param s The serializer to use (its mode decides whether this reads or writes).
         */
        void Serialize(Serializer &s)
        {
            // the enum goes through a plain uint16_t, since the serializer only handles arithmetic types
            uint16_t t = (uint16_t)type;
            s.field("type", t);
            type = (ItemType)t;
            s.field("amount", amount);
        }
    };
}