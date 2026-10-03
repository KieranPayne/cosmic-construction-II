#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "../imgui/imgui.h"
namespace cc
{
    class Entity;
    std::vector<std::string> Split(std::string str, char splitChar);
    sf::Color HexToColor(const std::string &hex);
    sf::Vector2f JsonAsVector(nlohmann::json &j);
    int TileToChunkPos(int pos);
    sf::Vector2i TileToChunkPos(sf::Vector2i &pos);
    sf::Vector2i TileToChunkPos(sf::Vector2f &pos);
    ImGuiKey keycodeToImGuiKey(sf::Keyboard::Key key);
    std::vector<uint8_t> ReadBytesFromPacket(sf::Packet& packet);
    void AppendEntityToPacket(sf::Packet& packet,Entity* e);
    Entity* LoadEntityFromPacket(sf::Packet& packet);
    sf::Color UsernameToColor(std::string& username);
    float Lerp(float a, float b,float t);
    sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t);
}