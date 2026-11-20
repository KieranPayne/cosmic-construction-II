#pragma once
#include "../PCH.hpp"
#include "ItemData.hpp"
#include "Item.hpp"
namespace cc
{

    class Inventory
    {
        private:
        int numSlots;
        public:
        std::vector<ItemData> data;
        Inventory();
        void SetNumSlots(int numSlots);
        int GetNumSlots();
        void Serialize(Serializer& s);
        //attempts to add the item data to the inventory, returns false if no slots available
        bool AddItem(ItemData* itemData);
    };
}