#pragma once
#include "Entity.hpp"
#include "ItemData.hpp"
namespace cc
{
    /**
     * @brief An item lying in the world, as an entity.
     *
     * What the item is, and how many of it there are, is held in `itemData`.
     */
    class Item : public Entity
    {
    public:
        /// What kind of item this is and how many it represents.
        ItemData itemData;

        /// @brief Creates an item entity. Sets the entity's type to ITEM.
        Item();

        /**
         * @brief Gets where this item's sprite is in the entity texture atlas.
         * @return The sprite's top-left corner, looked up from the item's type in the "Item" entry of the
         *         entity textures json.
         */
        sf::Vector2f GetTexCoords();

        /**
         * @brief Writes or reads the item: the base entity's fields, then `itemData`.
         * @param s The serializer to use (its mode decides whether this reads or writes).
         */
        void Serialize(Serializer &s);
    };
}