#include "KnowledgeDatabase.hpp"
#include "SaveManager.hpp"
namespace cc
{
    KnowledgeDatabase::KnowledgeDatabase()
    {
    }

    void KnowledgeDatabase::SaveToFile(std::string path)
    {
        Serializer s(Serializer::Mode::WRITE, Serializer::Format::JSON);
        s.field("nodes", nodes);
        s.field("connections", connections);
        SaveManager::WriteData(path, s.json().dump(2));
    }

    void KnowledgeDatabase::LoadFromFile(std::string path)
    {
        std::string data = SaveManager::ReadData(path);
        nlohmann::json j = nlohmann::json::parse(data);
        Serializer s(Serializer::Mode::READ, Serializer::Format::JSON, j);
        nodes = {};
        connections = {};
        s.field("nodes", nodes);
        s.field("connections", connections);
        BuildOutgoingAndIncoming();
    }

    void KnowledgeDatabase::BuildOutgoingAndIncoming()
    {
        outgoingConnections.assign(nodes.size(), {});
        incomingConnections.assign(nodes.size(), {});
        for (int j = 0; j < static_cast<int>(connections.size()); j++)
        {
            int start = connections[j].start;
            int end = connections[j].end;
            if (start >= 0 && start < static_cast<int>(nodes.size()))
            {
                outgoingConnections[start].push_back(j);
            }
            if (end >= 0 && end < static_cast<int>(nodes.size()))
            {
                incomingConnections[end].push_back(j);
            }
        }
    }

    int KnowledgeDatabase::AddNode(const KnowledgeNodeData &data)
    {
        nodes.push_back(data);
        BuildOutgoingAndIncoming();
        return static_cast<int>(nodes.size()) - 1;
    }

    int KnowledgeDatabase::AddConnection(int start, int end, float weight)
    {
        KnowledgeConnection c;
        c.start = start;
        c.end = end;
        c.weight = weight;
        connections.push_back(c);
        BuildOutgoingAndIncoming();
        return static_cast<int>(connections.size()) - 1;
    }

    void KnowledgeDatabase::RemoveConnection(int index)
    {
        connections.erase(connections.begin() + index);
        BuildOutgoingAndIncoming();
    }

    std::vector<int> KnowledgeDatabase::ConnectionsAttachedTo(int nodeIndex) const
    {
        std::vector<int> attached;
        for (int i = static_cast<int>(connections.size()) - 1; i >= 0; --i)
        {
            if (connections[i].TouchesNode(nodeIndex))
            {
                attached.push_back(i);
            }
        }
        return attached;
    }

    void KnowledgeDatabase::RemoveNode(int index)
    {
        // Attached connections go first (highest index first, so the others keep their indices).
        for (int i : ConnectionsAttachedTo(index))
        {
            connections.erase(connections.begin() + i);
        }

        // Every node above the removed one shifts down, so connections pointing at them must follow.
        for (auto &c : connections)
        {
            c.AdjustForRemovedNode(index);
        }

        nodes.erase(nodes.begin() + index);
        BuildOutgoingAndIncoming();
    }
}