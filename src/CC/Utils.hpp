#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "../imgui/imgui.h"
namespace cc
{
    std::vector<std::string> Split(std::string str, char splitChar);
    sf::Color HexToColor(const std::string &hex);
    sf::Vector2f JsonAsVector(nlohmann::json &j);
    int TileToChunkPos(int pos);
    sf::Vector2i TileToChunkPos(sf::Vector2i &pos);
    sf::Vector2i TileToChunkPos(sf::Vector2f &pos);
    ImGuiKey keycodeToImGuiKey(sf::Keyboard::Key key);
}