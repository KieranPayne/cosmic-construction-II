#include "KnowledgeConnection.hpp"

namespace cc
{
    KnowledgeConnection::KnowledgeConnection()
    {

    }

    bool KnowledgeConnection::TouchesNode(int nodeIndex) const
    {
        return start == nodeIndex || end == nodeIndex;
    }

    void KnowledgeConnection::AdjustForRemovedNode(int removedIndex)
    {
        if (start > removedIndex)
        {
            --start;
        }
        if (end > removedIndex)
        {
            --end;
        }
    }

    void KnowledgeConnection::Serialize(Serializer& s)
    {
        s.field("start",start);
        s.field("end",end);
        s.field("weight",weight);
    }
}