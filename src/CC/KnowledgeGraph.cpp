#include "KnowledgeGraph.hpp"

#include "Utils.hpp"
#include "../json.hpp"
#include "SaveManager.hpp"

namespace cc
{
    KnowledgeGraph::KnowledgeGraph(sf::RenderTarget *renderTarget, KnowledgeDatabase &db) : database(db)
    {
        this->renderTarget = renderTarget;
        // Change this path to your font file.
        fontLoaded = font.openFromFile("content/resources/fonts/default font.ttf");
        // LoadFromFile();
    }

    void KnowledgeGraph::Update(double dt, InputState &inputState)
    {
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({200, 0});
        ImGui::Begin("Choose Mode");
        // Edit mode dropdown
        ImGui::Text("Edit Mode");

        const char *editModes[] = {"Add", "Remove", "Move", "Edit Data"};
        int currentMode = static_cast<int>(editMode);

        if (ImGui::Combo("##test", &currentMode, editModes, IM_ARRAYSIZE(editModes)))
        {
            editMode = static_cast<EditMode>(currentMode);

            // Cancel any pending connection creation when changing modes.
            connectionStartIndex = -1;
            moving = false;
            selectedNodeIndex = -1;
            selectedConnectionIndex = -1;
            placingNode = false;
        }
        ImGui::End();

        // The canvas is locked while the New Node popup is open.
        if (!placingNode)
        {
            camera.Update(dt, inputState, renderTarget);
        }

        int hoverIndex = HoveringNodeIndex(inputState);

        if (editMode == EditMode::ADD)
        {
            // Clicks on the canvas are ignored while the new node's widget is open.
            if (!placingNode && inputState.Pressed(sf::Mouse::Button::Left) && !camera.GetPanning())
            {
                if (connectionStartIndex != -1)
                {
                    if (hoverIndex != -1)
                    {
                        Connection c(sf::Color::White);
                        c.dataIndex = database.AddConnection(connectionStartIndex, hoverIndex);
                        connections.push_back(c);

                        connectionStartIndex = -1;
                    }
                    else
                    {
                        connectionStartIndex = -1;
                    }
                }
                else
                {
                    if (hoverIndex == -1)
                    {
                        // Open the widget; the node is added when "Done" is pressed.
                        pendingNode = Node();
                        pendingData = KnowledgeNodeData();
                        pendingNode.position = camera.ToWorldPos(
                            inputState.mousePosition, renderTarget);
                        placingNode = true;
                        newNodeJustOpened = true;
                    }
                    else
                    {
                        connectionStartIndex = hoverIndex;
                    }
                }
            }

            if (placingNode)
            {
                DrawNewNodeEditor();
            }
        }
        else if (editMode == EditMode::REMOVE)
        {
            if (inputState.Pressed(sf::Mouse::Button::Left) && !camera.GetPanning())
            {
                // Prefer removing a node if the mouse is over one.
                if (hoverIndex != -1)
                {
                    // Removes the node and its connections, and shifts the indices above it.
                    RemoveNodeAt(hoverIndex);

                    selectedConnectionIndex = -1;

                    // Keep the editor selection pointing at the same node.
                    if (selectedNodeIndex == hoverIndex)
                    {
                        selectedNodeIndex = -1;
                    }
                    else if (selectedNodeIndex > hoverIndex)
                    {
                        --selectedNodeIndex;
                    }

                    // Cancel any pending connection creation.
                    connectionStartIndex = -1;
                }
                else
                {
                    // No node under the mouse; check for a connection.
                    int connectionIndex =
                        HoveringConnectionIndex(inputState);

                    if (connectionIndex != -1)
                    {
                        RemoveConnectionAt(connectionIndex);
                        selectedConnectionIndex = -1;
                    }
                }
            }
        }
        else if (editMode == EditMode::MOVE)
        {
            if (inputState.Pressed(sf::Mouse::Button::Left) && !camera.GetPanning())
            {
                if (hoverIndex != -1)
                {
                    moving = true;
                    moveIndex = hoverIndex;
                    moveMouseStartPos = camera.ToWorldPos(inputState.mousePosition, renderTarget);
                    moveNodeStartPos = NodeForData(hoverIndex)->position;
                }
            }
            if (moving)
            {
                sf::Vector2f newPos = camera.ToWorldPos(inputState.mousePosition, renderTarget);
                if (Node *moved = NodeForData(moveIndex))
                {
                    moved->position = moveNodeStartPos + newPos - moveMouseStartPos;
                }
                if (inputState.Released(sf::Mouse::Button::Left))
                {
                    moving = false;
                }
            }
        }
        else if (editMode == EditMode::EDIT)
        {
            if (inputState.Pressed(sf::Mouse::Button::Left) && !camera.GetPanning() &&
                !ImGui::GetIO().WantCaptureMouse)
            {
                // Clicking a node or connection selects it. Clicking empty space closes the editor.
                if (hoverIndex != -1)
                {
                    if (selectedNodeIndex != hoverIndex)
                    {
                        selectionChanged = true;
                    }
                    selectedNodeIndex = hoverIndex;
                    selectedConnectionIndex = -1;
                }
                else
                {
                    int connectionIndex = HoveringConnectionIndex(inputState);
                    if (connectionIndex != -1)
                    {
                        if (selectedConnectionIndex != connectionIndex)
                        {
                            selectionChanged = true;
                        }
                        selectedConnectionIndex = connectionIndex;
                        selectedNodeIndex = -1;
                    }
                    else
                    {
                        selectedNodeIndex = -1;
                        selectedConnectionIndex = -1;
                    }
                }
            }
            DrawSelectionEditor(); // must be outside the if above, so it runs every frame
        }
    }

    void KnowledgeGraph::DrawSelectionEditor()
    {
        if (selectedNodeIndex >= database.NodeCount())
        {
            selectedNodeIndex = -1;
        }
        if (selectedConnectionIndex >= database.ConnectionCount())
        {
            selectedConnectionIndex = -1;
        }
        if (selectedNodeIndex < 0 && selectedConnectionIndex < 0)
        {
            return;
        }

        bool open = true;
        ImGui::SetNextWindowPos({0, 100}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({300, 250}, ImGuiCond_Once);
        ImGui::Begin("Data Editor", &open);

        // Open the root tree node only on the frame the selection changes, so it can still be collapsed.
        bool openRoot = selectionChanged;
        selectionChanged = false;

        // Edits go straight into the database.
        if (selectedNodeIndex >= 0)
        {
            EditObject(database.GetNode(selectedNodeIndex), "node", openRoot);
        }
        else if (EditObject(database.GetConnection(selectedConnectionIndex), "connection", openRoot))
        {
            // The start or end may have changed.
            database.BuildOutgoingAndIncoming();
        }

        ImGui::End();

        if (!open)
        {
            selectedNodeIndex = -1;
            selectedConnectionIndex = -1;
        }
    }

    void KnowledgeGraph::DrawNewNodeEditor()
    {
        if (newNodeJustOpened)
        {
            ImGui::OpenPopup("New Node");
        }

        // Centre of the screen. The modal popup dims and blocks everything behind it.
        ImVec2 display = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(
            ImVec2(display.x * 0.5f, display.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(350, 350), ImGuiCond_Appearing);

        if (!ImGui::BeginPopupModal("New Node", nullptr, ImGuiWindowFlags_NoCollapse))
        {
            // Closed some other way (e.g. Escape): cancel.
            placingNode = false;
            return;
        }

        // Fields fill the window, leaving room for the Done button underneath.
        float footerHeight = ImGui::GetStyle().ItemSpacing.y * 2.f + ImGui::GetFrameHeight() + 1.f;
        ImGui::BeginChild("fields", ImVec2(0, -footerHeight));
        EditObject(pendingData, "node", newNodeJustOpened);
        ImGui::EndChild();

        newNodeJustOpened = false;

        ImGui::Separator();
        if (ImGui::Button("Done"))
        {
            pendingNode.dataIndex = database.AddNode(pendingData);
            nodes.push_back(pendingNode);
            placingNode = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void KnowledgeGraph::RefreshNodeNames()
    {
        jsonEditor.nodeNames.clear();
        for (int i = 0; i < database.NodeCount(); ++i)
        {
            jsonEditor.nodeNames.push_back(database.GetNode(i).GetName());
        }
    }

    void KnowledgeGraph::DrawNodeLabel(const Node &node, const KnowledgeNodeData &data)
    {
        if (data.GetName().empty())
        {
            return;
        }

        // The text is 16 world units tall. Rasterise the glyphs at the size they appear on screen
        // (zoom * 16 pixels), then scale back down to world units, so they stay sharp at any zoom.
        // Rounding to a multiple of 4 limits how many glyph sizes get cached.
        const float worldTextSize = 16.f;
        float zoom = static_cast<float>(renderTarget->getSize().x) /
                     renderTarget->getView().getSize().x;
        int pixelSize = static_cast<int>(std::lround(worldTextSize * zoom / 4.f)) * 4;
        pixelSize = std::clamp(pixelSize, 8, 256);

        sf::Text text(font, data.GetName(), pixelSize);
        text.setScale({worldTextSize / pixelSize, worldTextSize / pixelSize});
        text.setFillColor(sf::Color::White);

        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                        bounds.position.y + bounds.size.y});

        // Translucent black box behind the text. Sizes are in world units.
        const float padding = 3.f;
        float textWidth = bounds.size.x * text.getScale().x;
        float textHeight = bounds.size.y * text.getScale().y;

        sf::Vector2f textPos = {node.position.x,
                                node.position.y - node.radius - 4.f - padding};
        text.setPosition(textPos);

        sf::RectangleShape box({textWidth + padding * 2.f, textHeight + padding * 2.f});
        box.setPosition({textPos.x - textWidth / 2.f - padding,
                         textPos.y - textHeight - padding});
        box.setFillColor(sf::Color(0, 0, 0, 150));

        renderTarget->draw(box);
        renderTarget->draw(text);
    }

    void KnowledgeGraph::Render(InputState &inputState)
    {
        sf::View original = renderTarget->getView();

        // Dark grey background, drawn in screen space before the camera view is applied.
        renderTarget->setView(renderTarget->getDefaultView());
        sf::RectangleShape background(sf::Vector2f(renderTarget->getSize()));
        background.setFillColor(sf::Color(45, 45, 48));
        renderTarget->draw(background);

        camera.SetView(renderTarget);

        for (auto &c : connections)
        {
            const KnowledgeConnection &data = database.GetConnection(c.dataIndex);
            Node *startNode = NodeForData(data.start);
            Node *endNode = NodeForData(data.end);
            if (startNode == nullptr || endNode == nullptr)
            {
                continue;
            }

            c.Draw(
                renderTarget, data, *startNode, *endNode,
                HasReverseConnection(c.dataIndex),
                c.dataIndex == selectedConnectionIndex);
        }

        for (auto &n : nodes)
        {
            n.Draw(renderTarget, database.GetNode(n.dataIndex));
        }

        // The node waiting for "Done" in the New Node widget.
        if (placingNode)
        {
            pendingNode.Draw(renderTarget, pendingData);
        }

        // Node names above the nodes.
        if (fontLoaded)
        {
            for (auto &n : nodes)
            {
                DrawNodeLabel(n, database.GetNode(n.dataIndex));
            }
            if (placingNode)
            {
                DrawNodeLabel(pendingNode, pendingData);
            }
        }

        // Outline the node open in the editor.
        if (Node *selected = NodeForData(selectedNodeIndex))
        {
            sf::CircleShape outline(selected->radius);
            outline.setOrigin({selected->radius, selected->radius});
            outline.setPosition(selected->position);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::White);
            outline.setOutlineThickness(3.f);
            renderTarget->draw(outline);
        }

        // The connection being dragged out from a node to the mouse.
        if (Node *startNode = NodeForData(connectionStartIndex))
        {
            Node mouseNode;
            mouseNode.position = camera.ToWorldPos(
                inputState.mousePosition, renderTarget);
            mouseNode.radius = 0.f;

            KnowledgeConnection preview;
            preview.start = connectionStartIndex;

            Connection c(sf::Color::White);
            c.Draw(renderTarget, preview, *startNode, mouseNode);

            startNode->Draw(renderTarget, database.GetNode(connectionStartIndex));
        }

        renderTarget->setView(original);
    }

    KnowledgeGraph::Node *KnowledgeGraph::NodeForData(int dataIndex)
    {
        if (dataIndex < 0)
        {
            return nullptr;
        }

        // The visual list normally mirrors the database order, so try that first.
        if (dataIndex < static_cast<int>(nodes.size()) &&
            nodes[dataIndex].dataIndex == dataIndex)
        {
            return &nodes[dataIndex];
        }

        for (auto &n : nodes)
        {
            if (n.dataIndex == dataIndex)
            {
                return &n;
            }
        }
        return nullptr;
    }

    void KnowledgeGraph::RemoveConnectionAt(int dataIndex)
    {
        database.RemoveConnection(dataIndex);

        for (int i = static_cast<int>(connections.size()) - 1; i >= 0; --i)
        {
            if (connections[i].dataIndex == dataIndex)
            {
                connections.erase(connections.begin() + i);
            }
        }

        // Connections above the removed one have moved down in the database.
        for (auto &c : connections)
        {
            c.AdjustForRemovedConnection(dataIndex);
        }
    }

    void KnowledgeGraph::RemoveNodeAt(int dataIndex)
    {
        // Attached connections first (highest index first), so the visual list stays in step.
        for (int connectionIndex : database.ConnectionsAttachedTo(dataIndex))
        {
            RemoveConnectionAt(connectionIndex);
        }

        database.RemoveNode(dataIndex);

        for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i)
        {
            if (nodes[i].dataIndex == dataIndex)
            {
                nodes.erase(nodes.begin() + i);
            }
        }

        // Nodes above the removed one have moved down in the database.
        for (auto &n : nodes)
        {
            n.AdjustForRemovedNode(dataIndex);
        }
    }

    KnowledgeGraph::Node::Node()
    {
        this->position = {0.f, 0.f};
        radius = 20.f;
        circle = sf::CircleShape(radius);
    }

    void KnowledgeGraph::Node::Serialize(Serializer &s)
    {
        s.field("index", dataIndex);
        s.field("position", position);
    }

    void KnowledgeGraph::Node::AdjustForRemovedNode(int removedIndex)
    {
        if (dataIndex > removedIndex)
        {
            --dataIndex;
        }
    }

    void KnowledgeGraph::Node::Draw(sf::RenderTarget *target, const KnowledgeNodeData &data)
    {
        circle.setPosition(position);
        circle.setRadius(radius);
        circle.setOrigin({radius, radius});
        circle.setFillColor(data.GetColor());
        target->draw(circle);
    }

    KnowledgeGraph::Connection::Connection(sf::Color color)
    {
        thickness = 3.f;
        this->color = color;
    }

    void KnowledgeGraph::Connection::AdjustForRemovedConnection(int removedIndex)
    {
        if (dataIndex > removedIndex)
        {
            --dataIndex;
        }
    }

    sf::Vector2f KnowledgeGraph::Connection::OffsetVector(
        const Node &startNode, const Node &endNode) const
    {
        sf::Vector2f delta = endNode.position - startNode.position;

        if (delta.lengthSquared() == 0.f)
        {
            return {0.f, 0.f};
        }

        // Perpendicular to this connection's own direction, so the reverse
        // connection (opposite direction) is pushed to the opposite side.
        sf::Vector2f dir = delta.normalized();
        return sf::Vector2f(-dir.y, dir.x) * (thickness * 1.5f);
    }

    void KnowledgeGraph::Connection::Draw(
        sf::RenderTarget *target, const KnowledgeConnection &data,
        const Node &startNode, const Node &endNode,
        bool offsetLine, bool highlight)
    {
        float headLength = thickness / 3.f * 10.f;
        float headWidth = thickness / 3.f * 10.f;

        sf::Vector2f startPos = startNode.position;
        sf::Vector2f endPos = endNode.position;

        sf::Vector2f delta = endPos - startPos;

        if (delta.lengthSquared() == 0.f)
        {
            return;
        }

        sf::Vector2f dirVector = delta.normalized();

        if (offsetLine)
        {
            sf::Vector2f offset = OffsetVector(startNode, endNode);
            startPos += offset;
            endPos += offset;
        }

        startPos += dirVector * (startNode.radius - thickness);
        endPos -= dirVector * endNode.radius;
        sf::Color minWeightColor(color.r, color.g, color.b, 100);
        sf::Color drawColor = Lerp(minWeightColor, color, data.weight);
        DrawArrow(
            target, startPos, endPos,
            highlight ? sf::Color::Yellow : drawColor,
            thickness, headLength, headWidth);
    }

    bool KnowledgeGraph::HasReverseConnection(int index)
    {
        const KnowledgeConnection &c = database.GetConnection(index);
        for (int j = 0; j < database.ConnectionCount(); ++j)
        {
            const KnowledgeConnection &other = database.GetConnection(j);
            if (j != index && other.start == c.end && other.end == c.start)
            {
                return true;
            }
        }
        return false;
    }

    void KnowledgeGraph::LoadFromFile()
    {
        std::string path;
        if (SaveManager::debuggingKnowledgeGraph)
        {
            path = SaveManager::GetSavedataDir() + "/knowledge layout.json";
            if (!SaveManager::FileExists(path))
            {
                path = "content/resources/knowledge layout.json";
            }
        }
        else
        {
            path = "content/resources/knowledge layout.json";
        }

        nodes = {};
        connections = {};
        // database = KnowledgeDatabase();

        // Positions come from the layout file.
        std::vector<Node> layout;
        nlohmann::json json = nlohmann::json::parse(SaveManager::ReadData(path));
        Serializer s(Serializer::Mode::READ, Serializer::Format::JSON, json);
        s.field("nodes", layout);

        // One visual node per database node, in database order. Any without a saved position sit at the origin.
        for (int i = 0; i < database.NodeCount(); ++i)
        {
            Node n;
            n.dataIndex = i;
            for (auto &saved : layout)
            {
                if (saved.dataIndex == i)
                {
                    n.position = saved.position;
                    break;
                }
            }
            nodes.push_back(n);
        }

        for (int i = 0; i < database.ConnectionCount(); ++i)
        {
            Connection c(sf::Color::White);
            c.dataIndex = i;
            connections.push_back(c);
        }
    }

    void KnowledgeGraph::LoadOldFormat(const std::string &path)
    {
        nlohmann::json json = nlohmann::json::parse(SaveManager::ReadData(path));

        for (auto &j : json["nodes"])
        {
            Serializer s(Serializer::Mode::READ, Serializer::Format::JSON, j);

            KnowledgeNodeData data;
            Node n;
            s.field("position", n.position);
            s.field("data", data);
            n.dataIndex = database.AddNode(data);
            nodes.push_back(n);
        }

        for (auto &j : json["connections"])
        {
            Serializer s(Serializer::Mode::READ, Serializer::Format::JSON, j);

            KnowledgeConnection data;
            s.field("data", data);

            Connection c(sf::Color::White);
            c.dataIndex = database.AddConnection(data.start, data.end, data.weight);
            connections.push_back(c);
        }
    }

    void KnowledgeGraph::SaveToFile()
    {

        // Where each node sits in the editor.
        Serializer s(Serializer::Mode::WRITE, Serializer::Format::JSON);
        s.field("nodes", nodes);
        std::string savePath = SaveManager::GetSavedataDir() + "/knowledge layout.json";

        SaveManager::WriteData(savePath, s.json().dump(2));
    }

    int KnowledgeGraph::HoveringNodeIndex(InputState &inputState)
    {
        sf::Vector2f worldMousePos = camera.ToWorldPos(
            inputState.mousePosition, renderTarget);

        // Returns the node's database index.
        for (auto &n : nodes)
        {
            if (n.TouchingPoint(worldMousePos))
            {
                return n.dataIndex;
            }
        }

        return -1;
    }

    bool KnowledgeGraph::Node::TouchingPoint(sf::Vector2f point)
    {
        if ((position - point).length() <= radius)
        {
            return true;
        }

        return false;
    }

    int KnowledgeGraph::HoveringConnectionIndex(InputState &inputState)
    {
        sf::Vector2f mouseWorld = camera.ToWorldPos(
            inputState.mousePosition, renderTarget);

        // Check in reverse order so connections drawn last are selected first.
        // Returns the connection's database index.
        for (int i = static_cast<int>(connections.size()) - 1;
             i >= 0; --i)
        {
            const Connection &connection = connections[i];
            int dataIndex = connection.dataIndex;
            const KnowledgeConnection &data = database.GetConnection(dataIndex);

            Node *startNode = NodeForData(data.start);
            Node *endNode = NodeForData(data.end);
            if (startNode == nullptr || endNode == nullptr)
            {
                continue;
            }

            // Match the sideways shift used when drawing reciprocal connections.
            sf::Vector2f offset = HasReverseConnection(dataIndex)
                                      ? connection.OffsetVector(*startNode, *endNode)
                                      : sf::Vector2f(0.f, 0.f);

            sf::Vector2f start = startNode->position + offset;
            sf::Vector2f end = endNode->position + offset;

            sf::Vector2f segment = end - start;
            sf::Vector2f toMouse = mouseWorld - start;

            float lengthSquared =
                segment.x * segment.x + segment.y * segment.y;

            // Handle overlapping nodes.
            if (lengthSquared == 0.f)
            {
                float dx = mouseWorld.x - start.x;
                float dy = mouseWorld.y - start.y;

                if (dx * dx + dy * dy <=
                    connection.thickness * connection.thickness / 4.f)
                {
                    return dataIndex;
                }

                continue;
            }

            // Project the mouse position onto the segment.
            float t = (toMouse.x * segment.x +
                       toMouse.y * segment.y) /
                      lengthSquared;

            // Clamp to the segment endpoints.
            t = std::clamp(t, 0.f, 1.f);

            sf::Vector2f closestPoint = start + segment * t;

            float dx = mouseWorld.x - closestPoint.x;
            float dy = mouseWorld.y - closestPoint.y;

            float distanceSquared = dx * dx + dy * dy;
            float halfThickness = connection.thickness / 2.f;

            if (distanceSquared <= halfThickness * halfThickness)
            {
                return dataIndex;
            }
        }

        return -1;
    }
}