#pragma once
#include "../State.hpp"
#include "../PCH.hpp"
struct ImGuiInputTextCallbackData;

namespace cc
{
    /**
     * @brief The screen where the player types their username. Shown on first launch.
     *
     * Continuing saves the name and moves on to the main menu.
     */
    class UsernameMenu : public Kosmic::State
    {
    public:
        /// The name being typed: up to 20 characters plus the null terminator.
        char username[21];

        /// Message shown in red under the field. Empty when there is nothing to show.
        std::string errorMessage;

        /**
         * @brief Creates the menu with an empty name.
         * @param target The window or texture the menu is drawn to. Handed on to the main menu.
         */
        UsernameMenu(sf::RenderTarget *target);

        /**
         * @brief ImGui filter that blocks typing any character other than a letter, a number or a space.
         * @param data The text field's callback data.
         * @return 1 to discard the typed character, 0 to accept it.
         */
        static int InputTextCallback(ImGuiInputTextCallbackData *data);

        /**
         * @brief Checks that a username is allowed.
         * @param name The name to check.
         * @return true if it is 1 to 20 characters long and only has letters, numbers and spaces.
         */
        bool IsValidUsername(const std::string &name);

        /// @brief Draws the menu. Continue checks the name, then saves it and moves on, or sets `errorMessage`.
        void DerivedUpdate();

        ~UsernameMenu();

        /**
         * @brief Records the chosen username and switches to the main menu.
         * @param username The name to use for this session.
         */
        void UsernameEntered(std::string username);

        /**
         * @brief Looks for a previously saved, valid username.
         * @return true if one was found; it is also stored as the session's username.
         */
        bool CheckForExisting();
    };
}