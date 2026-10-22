#pragma once
#include "../PCH.hpp"
#include "Camera.hpp"
#include "../Input/Input.hpp"
#include "Serializer.hpp"
#include "JsonEditor.hpp"
#include "KnowledgeEditor.hpp"
#include "UnlockRequirement.hpp"
#include "KnowledgeNodeData.hpp"
#include "KnowledgeConnection.hpp"
#include "KnowledgeDatabase.hpp"
namespace cc
{

    class KnowledgeGraph
    {
    private:
        /// The editor's visual side of a node: where it is and how it is drawn. Everything about the
        /// technology itself lives in the database, found through `dataIndex`.
        class Node
        {
        private:
            sf::CircleShape circle;

        public:
            /// Index of this node's data in the database. -1 for the pending node, which isn't in it yet.
            int dataIndex = -1;
            float radius;
            sf::Vector2f position;
            Node();
            void Draw(sf::RenderTarget *target, const KnowledgeNodeData &data);
            /// Saves only the layout (database index and position). The data is saved by the database.
            void Serialize(Serializer &s);
            bool TouchingPoint(sf::Vector2f point);
            /// Call after a node is removed from the database: a node above `removedIndex` moves down by one.
            void AdjustForRemovedNode(int removedIndex);
        };
        /// The editor's visual side of a connection. Its start, end and weight live in the database.
        class Connection
        {
        private:
            sf::Color color;

        public:
            /// Index of this connection in the database.
            int dataIndex = -1;
            float thickness;
            Connection(sf::Color color);
            /// @param offsetLine Shift the line sideways so it doesn't overlap a connection going the other way.
            /// @param highlight Draw in the selection colour.
            void Draw(sf::RenderTarget *target, const KnowledgeConnection &data,
                      const Node &startNode, const Node &endNode,
                      bool offsetLine = false, bool highlight = false);
            /// Sideways shift applied when this connection has a reverse twin. Opposite for the twin.
            sf::Vector2f OffsetVector(const Node &startNode, const Node &endNode) const;
            /// Call after a connection is removed from the database: one above `removedIndex` moves down by one.
            void AdjustForRemovedConnection(int removedIndex);
        };
        enum class EditMode : int
        {
            ADD = 0,
            REMOVE,
            MOVE,
            EDIT
        };
        EditMode editMode = EditMode::ADD;
        bool moving = false;
        sf::Vector2f moveMouseStartPos;
        sf::Vector2f moveNodeStartPos;
        int moveIndex;
        int connectionStartIndex = -1;
        /// Node shown in the data editor widget (EDIT mode). -1 when none is selected.
        int selectedNodeIndex = -1;
        /// Connection shown in the data editor widget (EDIT mode). -1 when none is selected.
        int selectedConnectionIndex = -1;
        /// True when the selection just changed, so the editor opens its root tree node once.
        bool selectionChanged = false;
        /// Draws any node's data as editable fields, so the widget never needs updating when the data changes.
        KnowledgeEditor jsonEditor;
        /// True while a newly added node is waiting in the "New Node" widget for "Done".
        bool placingNode = false;
        /// True for the first frame of the "New Node" popup, to open it and its root tree node.
        bool newNodeJustOpened = false;
        /// The node being set up in the "New Node" widget. Not in `nodes` until "Done" is pressed.
        Node pendingNode;
        /// The data being set up for the pending node. Not in the database until "Done" is pressed.
        KnowledgeNodeData pendingData;
        /// Font for node name labels. Labels are skipped if it failed to load.
        sf::Font font;
        bool fontLoaded = false;

        /// Shows any object with a Serialize(Serializer&) as editable fields and writes changes back into it.
        template <typename T>
        bool EditObject(T &object, const char *label, bool openRoot = false)
        {
            RefreshNodeNames();
            Serializer sw(Serializer::Mode::WRITE, Serializer::Format::JSON);
            object.Serialize(sw);
            nlohmann::json j = sw.json();

            if (jsonEditor.DrawJson(j, label, openRoot))
            {
                Serializer sr(Serializer::Mode::READ, Serializer::Format::JSON, j);
                object.Serialize(sr);
                return true;
            }
            return false;
        }
        /// All the technology data. The editor reads and edits this directly.
        KnowledgeDatabase database;
        Camera camera;
        /// Visual nodes and connections. Each points at its database entry by index.
        std::vector<Node> nodes;
        std::vector<Connection> connections;
        sf::RenderTarget *renderTarget;
        int HoveringNodeIndex(InputState &inputState);
        int HoveringConnectionIndex(InputState &inputState);
        /// True if another connection runs between the same two nodes in the opposite direction.
        bool HasReverseConnection(int index);
        /// Draws the "Data Editor" window for the selected node or connection.
        void DrawSelectionEditor();
        /// Draws the "New Node" window for `pendingNode`. "Done" adds it to the graph.
        void DrawNewNodeEditor();
        /// Draws a node's name centred above it.
        void DrawNodeLabel(const Node &node, const KnowledgeNodeData &data);
        /// Gives the editor the current node names, for its node dropdowns.
        void RefreshNodeNames();
        /// The visual node showing database node `dataIndex`, or null if there isn't one.
        Node *NodeForData(int dataIndex);
        /// Removes a node and its connections from the database and the visuals, shifting indices above it.
        void RemoveNodeAt(int dataIndex);
        /// Removes a connection from the database and the visuals, shifting indices above it.
        void RemoveConnectionAt(int dataIndex);
        /// Loads the old single-file format (position and data together) into the database and visuals.
        void LoadOldFormat(const std::string &path);

    public:
        KnowledgeGraph(sf::RenderTarget *renderTarget);
        void LoadFromFile();
        void SaveToFile();
        void Update(double dt, InputState &inputState);
        void Render(InputState &inputState);
    };
}