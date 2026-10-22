#include "KnowledgeGraph.hpp"

#include "Utils.hpp"
#include "../json.hpp"
#include "SaveManager.hpp"

namespace cc
{
    KnowledgeGraph::KnowledgeGraph(sf::RenderTarget *renderTarget)
    {
        this->renderTarget = renderTarget;
        // Change this path to your font file.
        fontLoaded = font.openFromFile("content/resources/fonts/default font.ttf");
        LoadFromFile();
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
            camera.Update(dt, inputState);
        }

        int hoverIndex = HoveringNodeIndex(inputState);

        if (editMode == EditMode::ADD)
        {
            // Clicks on the canvas are ignored while the new node's widget is open.
            if (!placingNode && inputState.Pressed(sf::Mouse::Button::Left))
            {
                if (connectionStartIndex != -1)
                {
                    if (hoverIndex != -1)
                    {
                        Connection c(sf::Color::White);
                        c.start = connectionStartIndex;
                        c.end = hoverIndex;
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
            if (inputState.Pressed(sf::Mouse::Button::Left))
            {
                // Prefer removing a node if the mouse is over one.
                if (hoverIndex != -1)
                {
                    // Remove every connection attached to this node.
                    for (int i = static_cast<int>(connections.size()) - 1;
                         i >= 0; --i)
                    {
                        if (connections[i].start == hoverIndex ||
                            connections[i].end == hoverIndex)
                        {
                            connections.erase(connections.begin() + i);
                        }
                        else
                        {
                            // Adjust indices for the node that will shift
                            // into the removed node's position.
                            if (connections[i].start > hoverIndex)
                            {
                                --connections[i].start;
                            }

                            if (connections[i].end > hoverIndex)
                            {
                                --connections[i].end;
                            }
                        }
                    }

                    // Remove the node itself.
                    nodes.erase(nodes.begin() + hoverIndex);

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
                        connections.erase(
                            connections.begin() + connectionIndex);
                        selectedConnectionIndex = -1;
                    }
                }
            }
        }
        else if (editMode == EditMode::MOVE)
        {
            if (inputState.Pressed(sf::Mouse::Button::Left))
            {
                if (hoverIndex != -1)
                {
                    moving = true;
                    moveIndex = hoverIndex;
                    moveMouseStartPos = camera.ToWorldPos(inputState.mousePosition, renderTarget);
                    moveNodeStartPos = nodes[hoverIndex].position;
                }
            }
            if (moving)
            {
                sf::Vector2f newPos = camera.ToWorldPos(inputState.mousePosition, renderTarget);
                nodes[moveIndex].position = moveNodeStartPos + newPos - moveMouseStartPos;
                if (inputState.Released(sf::Mouse::Button::Left))
                {
                    moving = false;
                }
            }
        }
        else if (editMode == EditMode::EDIT)
        {
            // Ignore clicks that land on an ImGui window (including the node editor itself).
            if (inputState.Pressed(sf::Mouse::Button::Left) &&
                !ImGui::GetIO().WantCaptureMouse)
            {
                // Clicking empty space keeps the current selection; close the widget to deselect.
                if (hoverIndex != -1)
                {
                    selectedNodeIndex = hoverIndex;
                    selectedConnectionIndex = -1;
                }
                else
                {
                    int connectionIndex = HoveringConnectionIndex(inputState);
                    if (connectionIndex != -1)
                    {
                        selectedConnectionIndex = connectionIndex;
                        selectedNodeIndex = -1;
                    }
                }
            }
            DrawSelectionEditor();
        }
    }

    void KnowledgeGraph::DrawSelectionEditor()
    {
        if (selectedNodeIndex >= static_cast<int>(nodes.size()))
        {
            selectedNodeIndex = -1;
        }
        if (selectedConnectionIndex >= static_cast<int>(connections.size()))
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

        if (selectedNodeIndex >= 0)
        {
            EditObject(nodes[selectedNodeIndex], "node");
        }
        else
        {
            EditObject(connections[selectedConnectionIndex], "connection");
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
        EditObject(pendingNode, "node", newNodeJustOpened);
        ImGui::EndChild();

        newNodeJustOpened = false;

        ImGui::Separator();
        if (ImGui::Button("Done"))
        {
            nodes.push_back(pendingNode);
            placingNode = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void KnowledgeGraph::DrawNodeLabel(Node &node)
    {
        if (node.Name().empty())
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

        sf::Text text(font, node.Name(), pixelSize);
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

        for (int i = 0; i < static_cast<int>(connections.size()); ++i)
        {
            connections[i].Draw(
                renderTarget, nodes,
                HasReverseConnection(i),
                i == selectedConnectionIndex);
        }

        for (auto &n : nodes)
        {
            n.Draw(renderTarget);
        }

        // The node waiting for "Done" in the New Node widget.
        if (placingNode)
        {
            pendingNode.Draw(renderTarget);
        }

        // Node names above the nodes.
        if (fontLoaded)
        {
            for (auto &n : nodes)
            {
                DrawNodeLabel(n);
            }
            if (placingNode)
            {
                DrawNodeLabel(pendingNode);
            }
        }

        // Outline the node open in the editor.
        if (selectedNodeIndex >= 0 &&
            selectedNodeIndex < static_cast<int>(nodes.size()))
        {
            Node &selected = nodes[selectedNodeIndex];
            sf::CircleShape outline(selected.radius);
            outline.setOrigin({selected.radius, selected.radius});
            outline.setPosition(selected.position);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::White);
            outline.setOutlineThickness(3.f);
            renderTarget->draw(outline);
        }

        if (connectionStartIndex != -1)
        {
            Connection c(sf::Color::White);
            Node n;

            n.position = camera.ToWorldPos(
                inputState.mousePosition, renderTarget);
            n.radius = 0.f;

            nodes.push_back(n);

            c.start = connectionStartIndex;
            c.end = static_cast<int>(nodes.size()) - 1;

            c.Draw(renderTarget, nodes);

            nodes.pop_back();
            nodes[c.start].Draw(renderTarget);
        }

        renderTarget->setView(original);
    }

    KnowledgeGraph::Node::Node()
    {
        this->position = {0.f, 0.f};
        radius = 20.f;
        circle = sf::CircleShape(radius);
    }

    void KnowledgeGraph::Node::Serialize(Serializer &s)
    {
        s.field("position", position);
        s.field("data",data);
    }

    void KnowledgeGraph::Node::Draw(sf::RenderTarget *target)
    {
        circle.setPosition(position);
        circle.setRadius(radius);
        circle.setOrigin({radius, radius});
        circle.setFillColor(data.color);
        target->draw(circle);
    }

    KnowledgeGraph::Connection::Connection(sf::Color color)
    {
        thickness = 3.f;
        start = 0;
        end = 0;
        this->color = color;
    }

    void KnowledgeGraph::Connection::Serialize(Serializer &s)
    {
        s.field("start", start);
        s.field("end", end);
        s.field("weight", weight);
    }

    sf::Vector2f KnowledgeGraph::Connection::OffsetVector(
        const std::vector<Node> &nodes) const
    {
        if (start < 0 || start >= static_cast<int>(nodes.size()) ||
            end < 0 || end >= static_cast<int>(nodes.size()))
        {
            return {0.f, 0.f};
        }

        sf::Vector2f delta = nodes[end].position - nodes[start].position;

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
        sf::RenderTarget *target, std::vector<Node> &nodes,
        bool offsetLine, bool highlight)
    {
        if (start < 0 || start >= static_cast<int>(nodes.size()) ||
            end < 0 || end >= static_cast<int>(nodes.size()))
        {
            return;
        }

        float headLength = thickness / 3.f * 10.f;
        float headWidth = thickness / 3.f * 10.f;

        sf::Vector2f startPos = nodes[start].position;
        sf::Vector2f endPos = nodes[end].position;

        sf::Vector2f delta = endPos - startPos;

        if (delta.lengthSquared() == 0.f)
        {
            return;
        }

        sf::Vector2f dirVector = delta.normalized();

        if (offsetLine)
        {
            sf::Vector2f offset = OffsetVector(nodes);
            startPos += offset;
            endPos += offset;
        }

        startPos += dirVector * (nodes[start].radius - thickness);
        endPos -= dirVector * nodes[end].radius;

        DrawArrow(
            target, startPos, endPos,
            highlight ? sf::Color::Yellow : color,
            thickness, headLength, headWidth);
    }

    bool KnowledgeGraph::HasReverseConnection(int index)
    {
        for (int j = 0; j < static_cast<int>(connections.size()); ++j)
        {
            if (j != index &&
                connections[j].start == connections[index].end &&
                connections[j].end == connections[index].start)
            {
                return true;
            }
        }
        return false;
    }

    void KnowledgeGraph::LoadFromFile()
    {
        std::string path =
            SaveManager::GetSavedataDir() + "/knowledge graph.json";
        if (!SaveManager::FileExists(path))
        {
            nodes = {};
            connections = {};
        }else
        {

            // std::string path = "content/resources/knowledge graph.json";
            std::string data = SaveManager::ReadData(path);
            nlohmann::json json = nlohmann::json::parse(data);
            
            for (auto &j : json["nodes"])
            {
                Serializer s(
                Serializer::Mode::READ,
                Serializer::Format::JSON, j);

                Node n;
                n.Serialize(s);
                nodes.push_back(n);
            }
            
            for (auto &j : json["connections"])
            {
                Serializer s(
                Serializer::Mode::READ,
                Serializer::Format::JSON, j);

                Connection c(sf::Color::White);
                c.Serialize(s);
                connections.push_back(c);
            }
        }
    }

    void KnowledgeGraph::SaveToFile()
    {
        std::string path =
            SaveManager::GetSavedataDir() + "/knowledge graph.json";

        // std::string path = "content/resources/knowledge graph.json";
        nlohmann::json json;

        json["nodes"] = {};

        for (auto &n : nodes)
        {
            Serializer s(
                Serializer::Mode::WRITE,
                Serializer::Format::JSON);

            n.Serialize(s);
            json["nodes"].push_back(s.json());
        }

        json["connections"] = {};

        for (auto &c : connections)
        {
            Serializer s(
                Serializer::Mode::WRITE,
                Serializer::Format::JSON);

            c.Serialize(s);
            json["connections"].push_back(s.json());
        }

        SaveManager::WriteData(path, json.dump(2));
    }

    int KnowledgeGraph::HoveringNodeIndex(InputState &inputState)
    {
        sf::Vector2f worldMousePos = camera.ToWorldPos(
            inputState.mousePosition, renderTarget);

        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
        {
            if (nodes[i].TouchingPoint(worldMousePos))
            {
                return i;
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
        for (int i = static_cast<int>(connections.size()) - 1;
             i >= 0; --i)
        {
            const Connection &connection = connections[i];

            if (connection.start < 0 ||
                connection.start >= static_cast<int>(nodes.size()) ||
                connection.end < 0 ||
                connection.end >= static_cast<int>(nodes.size()))
            {
                continue;
            }

            // Match the sideways shift used when drawing reciprocal connections.
            sf::Vector2f offset = HasReverseConnection(i)
                                      ? connection.OffsetVector(nodes)
                                      : sf::Vector2f(0.f, 0.f);

            sf::Vector2f start = nodes[connection.start].position + offset;
            sf::Vector2f end = nodes[connection.end].position + offset;

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
                    return i;
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
                return i;
            }
        }

        return -1;
    }
    KnowledgeNodeData::KnowledgeNodeData()
    {

    }
    void KnowledgeNodeData::Serialize(Serializer& s)
    {
        s.field("name",name);
        s.field("description",description);
        s.field("color",color);
    }
}