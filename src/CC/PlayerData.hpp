#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "Serializer.hpp"
namespace cc
{
    /**
     * @brief Everything saved about one player: who they are and what they were looking at.
     *
     * The server keeps one of these per player (matched by username), so a returning player
     * resumes on the same planet with the same view.
     */
    class PlayerData
    {
    public:
        /// @brief Creates player data with defaults: a 1920x1080 window, no username, camera at the origin at zoom 1, planet 0.
        PlayerData()
        {
            resolution = {1920.f, 1080.f};
            username = "";
            cameraPosition = {0.f, 0.f};
            cameraZoom = 1.f;
            planet = 0;
        }

        /// Size in pixels of the player's window. With `cameraZoom`, decides which chunks the server sends when they join.
        sf::Vector2f resolution;

        /// The player's name. Identifies the player across sessions; an empty name means unregistered.
        std::string username;

        /// Where the centre of the player's view is, in world pixels.
        sf::Vector2f cameraPosition;

        /// How zoomed out the camera is. Larger values show more of the world.
        float cameraZoom;

        /// Index of the planet the player is on (an index into the server's planets).
        int planet;

        /**
         * @brief Writes or reads all of the player's data.
         * @param s The serializer to use (its mode decides whether this reads or writes).
         */
        void Serialize(Serializer &s)
        {
            s.field("resolution", resolution);
            s.field("username", username);
            s.field("cameraPosition", cameraPosition);
            s.field("cameraZoom", cameraZoom);
            s.field("planet", planet);
        }
    };
}