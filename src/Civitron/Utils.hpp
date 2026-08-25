#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "../imgui/imgui.h"
namespace Civitron
{
    std::vector<std::string> Split(std::string str, char splitChar);
    sf::Color HexToColor(const std::string &hex);
    sf::Vector2f JsonAsVector(nlohmann::json &j);
    int TileToChunkPos(int pos);
    sf::Vector3i TileToChunkPos(sf::Vector3i &pos);
    ImGuiKey keycodeToImGuiKey(sf::Keyboard::Key key);
}