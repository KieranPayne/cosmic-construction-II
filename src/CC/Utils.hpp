#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "../imgui/imgui.h"
namespace cc
{
    class Entity;

    /**
     * @brief Splits text at every occurrence of a character.
     * @param str The text to split.
     * @param splitChar The character to split at. It is not included in the pieces.
     * @return The pieces in order, including empty ones. Always has at least one piece.
     */
    std::vector<std::string> Split(std::string str, char splitChar);

    /**
     * @brief Turns a hex colour string into a colour.
     * @param hex "RRGGBB" or "RRGGBBAA", with or without a leading '#'. Alpha is 255 if left out.
     * @return The colour.
     * @throws std::invalid_argument if the string is the wrong length.
     */
    sf::Color HexToColor(const std::string &hex);

    /**
     * @brief Reads a two number json array, like [3, 4], as a vector.
     * @param j The json array. Both numbers must be integers.
     * @return The vector (x, y).
     */
    sf::Vector2f JsonAsVector(nlohmann::json &j);

    /**
     * @brief Finds the chunk a tile coordinate is in on one axis.
     * @param pos The tile coordinate.
     * @return The chunk coordinate, rounded down so negatives work. Assumes chunks are 32 tiles wide.
     */
    int TileToChunkPos(int pos);

    /**
     * @brief Finds the chunk a tile is in.
     * @param pos The tile's position in tiles.
     * @return The chunk's position in chunks. Assumes chunks are 32 tiles wide.
     */
    sf::Vector2i TileToChunkPos(sf::Vector2i &pos);

    /**
     * @brief Finds the chunk a position is in, for positions that aren't whole tiles (such as entities).
     * @param pos The position in tiles.
     * @return The chunk's position in chunks. Assumes chunks are 32 tiles wide.
     */
    sf::Vector2i TileToChunkPos(sf::Vector2f &pos);

    /**
     * @brief Converts an SFML key to the matching ImGui key.
     * @param key The SFML key.
     * @return The ImGui key, or ImGuiKey_None if there isn't a match.
     */
    ImGuiKey keycodeToImGuiKey(sf::Keyboard::Key key);

    /**
     * @brief Reads a block of bytes from a packet: a uint64_t count, then that many bytes.
     * @param packet The packet to read from.
     * @return The bytes.
     */
    std::vector<uint8_t> ReadBytesFromPacket(sf::Packet &packet);

    /**
     * @brief Adds an entity to a packet: its type, then its binary-serialized data with a byte count.
     * @param packet The packet to add to.
     * @param e The entity to add.
     */
    void AppendEntityToPacket(sf::Packet &packet, Entity *e);

    /**
     * @brief Reads an entity written by AppendEntityToPacket().
     * @param packet The packet to read from.
     * @return A new entity (the caller owns it).
     */
    Entity *LoadEntityFromPacket(sf::Packet &packet);

    /**
     * @brief Picks a bright colour for a player from their name. The same name always gives the same colour.
     * @param username The player's name.
     * @return The colour.
     */
    sf::Color UsernameToColor(std::string &username);

    /**
     * @brief Blends between two numbers.
     * @param a The value when t is 0.
     * @param b The value when t is 1.
     * @param t How far from a to b, usually from 0 to 1.
     * @return The blended value.
     */
    float Lerp(float a, float b, float t);

    /**
     * @brief Blends between two vectors.
     * @param a The vector when t is 0.
     * @param b The vector when t is 1.
     * @param t How far from a to b, usually from 0 to 1.
     * @return The blended vector.
     */
    sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t);

    /**
     * @brief Converts a colour from hue, saturation and value to RGB.
     * @param h Hue in degrees. Wraps around, so 360 is the same as 0.
     * @param s Saturation from 0 to 1 (values outside are clamped).
     * @param v Value (brightness) from 0 to 1 (values outside are clamped).
     * @return The colour, fully opaque.
     */
    sf::Color HsvToRgb(float h, float s, float v);
}