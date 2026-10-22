#pragma once
#include "../PCH.hpp"
#include "Serializer.hpp"
namespace cc
{
    class KnowledgeConnection
    {
    public:
        int start = -1;
        int end = -1;
        /// How much skill/understanding passes along this connection.
        float weight = 1.f;
        KnowledgeConnection();
        /// True if this connection starts or ends at the given node.
        bool TouchesNode(int nodeIndex) const;
        /// Call after a node is removed from the database. Any node index above `removedIndex` moves down by one.
        /// Connections attached to the removed node should already have been deleted.
        void AdjustForRemovedNode(int removedIndex);
        void Serialize(Serializer &s);
    };
}