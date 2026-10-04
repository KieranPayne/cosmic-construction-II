
#include "KnowledgeGraph.hpp"

#include "Utils.hpp"
#include "../json.hpp"
#include "SaveManager.hpp"

namespace cc
{
    KnowledgeGraph::KnowledgeGraph(sf::RenderTarget *renderTarget)
    {
        this->renderTarget = renderTarget;
        LoadFromFile();
    }

    void KnowledgeGraph::Update(double dt, InputState &inputState)
    {
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({200, 0});
        ImGui::Begin("Choose Mode");
        // Edit mode dropdown
        ImGui::Text("Edit Mode");

        const char *editModes[] = {"Add", "Remove", "Move"};
        int currentMode = static_cast<int>(editMode);

        if (ImGui::Combo("", &currentMode, editModes, IM_ARRAYSIZE(editModes)))
        {
            editMode = static_cast<EditMode>(currentMode);

            // Cancel any pending connection creation when changing modes.
            connectionStartIndex = -1;
            moving = false;
        }
        ImGui::End();

        camera.Update(dt, inputState);

        int hoverIndex = HoveringNodeIndex(inputState);

        if (editMode == EditMode::ADD)
        {
            if (inputState.Pressed(sf::Mouse::Button::Left))
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
                        Node n(nodeColor);
                        n.position = camera.ToWorldPos(
                            inputState.mousePosition, renderTarget);
                        nodes.push_back(n);
                    }
                    else
                    {
                        connectionStartIndex = hoverIndex;
                    }
                }
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
                    moveMouseStartPos = camera.ToWorldPos(inputState.mousePosition, renderTarget);
                    moveNodeStartPos = nodes[hoverIndex].position;
                }
            }
            if (moving)
            {
                sf::Vector2f newPos = camera.ToWorldPos(inputState.mousePosition, renderTarget);
                nodes[hoverIndex].position = moveNodeStartPos + newPos - moveMouseStartPos;
                if (inputState.Released(sf::Mouse::Button::Left))
                {
                    moving = false;
                }
            }
        }
    }

    void KnowledgeGraph::Render(InputState &inputState)
    {
        sf::View original = renderTarget->getView();
        camera.SetView(renderTarget);

        for (auto &c : connections)
        {
            c.Draw(renderTarget, nodes);
        }

        for (auto &n : nodes)
        {
            n.Draw(renderTarget);
        }

        if (connectionStartIndex != -1)
        {
            Connection c(sf::Color::White);
            Node n(sf::Color::Transparent);

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

    KnowledgeGraph::Node::Node(sf::Color color)
    {
        this->color = color;
        this->position = {0.f, 0.f};
        radius = 20.f;
        circle = sf::CircleShape(radius);
    }

    void KnowledgeGraph::Node::Serialize(Serializer &s)
    {
        s.field("position", position);
    }

    void KnowledgeGraph::Node::Draw(sf::RenderTarget *target)
    {
        circle.setPosition(position);
        circle.setRadius(radius);
        circle.setOrigin({radius, radius});
        circle.setFillColor(color);
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
    }

    void KnowledgeGraph::Connection::Draw(
        sf::RenderTarget *target, std::vector<Node> &nodes)
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

        startPos += dirVector * (nodes[start].radius - thickness);
        endPos -= dirVector * nodes[end].radius;

        DrawArrow(
            target, startPos, endPos, color,
            thickness, headLength, headWidth);
    }

    void KnowledgeGraph::LoadFromFile()
    {
        std::string path =
            SaveManager::GetSavedataDir() + "/knowledge graph.json";

        // std::string path = "content/resources/knowledge graph.json";
        std::string data = SaveManager::ReadData(path);
        nlohmann::json json = nlohmann::json::parse(data);

        for (auto &j : json["nodes"])
        {
            Serializer s(
                Serializer::Mode::READ,
                Serializer::Format::JSON, j);

            Node n(nodeColor);
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

            sf::Vector2f start = nodes[connection.start].position;
            sf::Vector2f end = nodes[connection.end].position;

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
}