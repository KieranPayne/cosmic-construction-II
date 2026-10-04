#pragma once
#include "../PCH.hpp"
#include "Camera.hpp"
#include "../Input/Input.hpp"
#include "Serializer.hpp"
namespace cc
{

    class KnowledgeGraph
    {
    private:
        class Node
        {
        private:
            sf::Color color;
            sf::CircleShape circle;

        public:
            float radius;
            sf::Vector2f position;
            Node(sf::Color color);
            void Draw(sf::RenderTarget *target);
            void Serialize(Serializer &s);
            bool TouchingPoint(sf::Vector2f point);
        };
        class Connection
        {
        private:
            sf::Color color;

        public:
            int start;
            int end;
            float thickness;
            Connection(sf::Color color);
            void Draw(sf::RenderTarget *target, std::vector<Node> &nodes);
            void Serialize(Serializer &s);
        };
        enum class EditMode : int
        {
            ADD = 0,
            REMOVE,
            MOVE
        };
        EditMode editMode = EditMode::ADD;
        bool moving = false;
        sf::Vector2f moveMouseStartPos;
        sf::Vector2f moveNodeStartPos;
        int connectionStartIndex = -1;
        Camera camera;
        std::vector<Node> nodes;
        std::vector<Connection> connections;
        sf::RenderTarget *renderTarget;
        sf::Color nodeColor = sf::Color(227, 121, 16);
        int HoveringNodeIndex(InputState &inputState);
        int HoveringConnectionIndex(InputState& inputState);
    public:
        KnowledgeGraph(sf::RenderTarget *renderTarget);
        void LoadFromFile();
        void SaveToFile();
        void Update(double dt, InputState &inputState);
        void Render(InputState& inputState);
    };
}