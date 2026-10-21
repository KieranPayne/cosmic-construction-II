#pragma once
#include "../PCH.hpp"
#include "Camera.hpp"
#include "../Input/Input.hpp"
#include "Serializer.hpp"
#include "JsonEditor.hpp"
namespace cc
{
    class KnowledgeNodeData
    {
        private:
        std::string name = "";
        std::string description = "";
        public:
        sf::Color color = sf::Color(226,121,16);
        const std::string &GetName() const { return name; }
        KnowledgeNodeData();
        void Serialize(Serializer& s);
    };
    class KnowledgeGraph
    {
    private:
        class Node
        {
        private:
            sf::CircleShape circle;
            KnowledgeNodeData data;
            public:
            float radius;
            sf::Vector2f position;
            Node();
            void Draw(sf::RenderTarget *target);
            void Serialize(Serializer &s);
            bool TouchingPoint(sf::Vector2f point);
            const std::string &Name() const { return data.GetName(); }
        };
        class Connection
        {
        private:
            sf::Color color;

        public:
            int start;
            int end;
            float thickness;
            /// How much skill/understanding passes along this connection.
            float weight = 1.f;
            Connection(sf::Color color);
            /// @param offsetLine Shift the line sideways so it doesn't overlap a connection going the other way.
            /// @param highlight Draw in the selection colour.
            void Draw(sf::RenderTarget *target, std::vector<Node> &nodes,
                      bool offsetLine = false, bool highlight = false);
            /// Sideways shift applied when this connection has a reverse twin. Opposite for the twin.
            sf::Vector2f OffsetVector(const std::vector<Node> &nodes) const;
            void Serialize(Serializer &s);
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
        /// Draws any node's data as editable fields, so the widget never needs updating when the data changes.
        JsonEditor jsonEditor;
        /// True while a newly added node is waiting in the "New Node" widget for "Done".
        bool placingNode = false;
        /// True for the first frame of the "New Node" popup, to open it and its root tree node.
        bool newNodeJustOpened = false;
        /// The node being set up in the "New Node" widget. Not in `nodes` until "Done" is pressed.
        Node pendingNode;
        /// Font for node name labels. Labels are skipped if it failed to load.
        sf::Font font;
        bool fontLoaded = false;

        /// Shows any object with a Serialize(Serializer&) as editable fields and writes changes back into it.
        template <typename T>
        void EditObject(T &object, const char *label, bool openRoot = false)
        {
            Serializer sw(Serializer::Mode::WRITE, Serializer::Format::JSON);
            object.Serialize(sw);
            nlohmann::json j = sw.json();

            if (jsonEditor.DrawJson(j, label, openRoot))
            {
                Serializer sr(Serializer::Mode::READ, Serializer::Format::JSON, j);
                object.Serialize(sr);
            }
        }
        Camera camera;
        std::vector<Node> nodes;
        std::vector<Connection> connections;
        sf::RenderTarget *renderTarget;
        int HoveringNodeIndex(InputState &inputState);
        int HoveringConnectionIndex(InputState& inputState);
        /// True if another connection runs between the same two nodes in the opposite direction.
        bool HasReverseConnection(int index);
        /// Draws the "Data Editor" window for the selected node or connection.
        void DrawSelectionEditor();
        /// Draws the "New Node" window for `pendingNode`. "Done" adds it to the graph.
        void DrawNewNodeEditor();
        /// Draws a node's name centred above it.
        void DrawNodeLabel(Node &node);
    public:
        KnowledgeGraph(sf::RenderTarget *renderTarget);
        void LoadFromFile();
        void SaveToFile();
        void Update(double dt, InputState &inputState);
        void Render(InputState& inputState);
    };
}