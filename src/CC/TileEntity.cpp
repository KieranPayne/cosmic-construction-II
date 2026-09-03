#include "TileEntity.hpp"

namespace cc
{
    TileEntity* CreateTileEntityFromType(uint16_t type)
    {
        if (type == 0)
        {
            return new TileEntity();
        }
        return nullptr;
    }
}