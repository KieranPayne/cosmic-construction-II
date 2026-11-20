#include "Inventory.hpp"

namespace cc
{
    Inventory::Inventory()
    {
        numSlots = 0;
    }
    void Inventory::SetNumSlots(int numSlots)
    {
        while (this->numSlots < numSlots)
        {
            this->numSlots ++;
            data.push_back(ItemData());
        }
        while (this->numSlots > numSlots)
        {
            data.erase(data.end()-1);
            this->numSlots --;
        }
    }
    int Inventory::GetNumSlots()
    {
        return numSlots;
    }
    void Inventory::Serialize(Serializer& s)
    {
        s.field("numSlots",numSlots);
        s.field("data",data);
    }
    bool Inventory::AddItem(ItemData* itemData)
    {
        for (int i = 0; i < numSlots; i ++)
        {
            if (data[i].type == itemData->type)
            {
                data[i].amount += itemData->amount;
                return true;
            }else if (data[i].type == ItemData::ItemType::NONE)
            {
                data[i].type = itemData->type;
                data[i].amount = itemData->amount;
                return true;
            }
        }
        return false;
        
    }

}