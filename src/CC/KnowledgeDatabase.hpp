#pragma once
#include "KnowledgeNodeData.hpp"
#include "KnowledgeConnection.hpp"

namespace cc
{
    /**
     * @brief All technologies and the weighted connections between them.
     *
     * A node's index is its ID, and a connection refers to nodes by those indices. Removing a node
     * therefore shifts every higher index down by one (see RemoveNode).
     */
    class KnowledgeDatabase
    {
    private:
        std::vector<KnowledgeNodeData> nodes;
        std::vector<KnowledgeConnection> connections;
        /// For each node, the indices of the connections that start / end at it.
        std::vector<std::vector<int>> outgoingConnections;
        std::vector<std::vector<int>> incomingConnections;

    public:
        KnowledgeDatabase();
        void LoadFromFile(std::string path);
        void SaveToFile(std::string path);
        /// Rebuilds the outgoing/incoming lists. Call after editing a connection's start or end directly.
        void BuildOutgoingAndIncoming();

        int NodeCount() const { return static_cast<int>(nodes.size()); }
        int ConnectionCount() const { return static_cast<int>(connections.size()); }
        KnowledgeNodeData &GetNode(int index) { return nodes[index]; }
        const KnowledgeNodeData &GetNode(int index) const { return nodes[index]; }
        KnowledgeConnection &GetConnection(int index) { return connections[index]; }
        const KnowledgeConnection &GetConnection(int index) const { return connections[index]; }
        const std::vector<int> &OutgoingConnections(int nodeIndex) const { return outgoingConnections[nodeIndex]; }
        const std::vector<int> &IncomingConnections(int nodeIndex) const { return incomingConnections[nodeIndex]; }

        /// Adds a node and returns its index.
        int AddNode(const KnowledgeNodeData &data);
        /// Adds a connection and returns its index.
        int AddConnection(int start, int end, float weight = 1.f);
        /// Removes a connection. Higher connection indices move down by one.
        void RemoveConnection(int index);
        /// Removes a node and any connections attached to it. Higher node indices move down by one, and
        /// connections are updated to match.
        void RemoveNode(int index);
        /// Indices of the connections touching a node, highest first (so they can be removed in this order).
        std::vector<int> ConnectionsAttachedTo(int nodeIndex) const;
    };
}