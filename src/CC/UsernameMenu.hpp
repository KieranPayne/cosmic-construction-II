#pragma once
#include "../State.hpp"
#include "../PCH.hpp"
struct ImGuiInputTextCallbackData;

namespace cc
{
    class UsernameMenu : public Kosmic::State
    {
    public:
        char username[21]; // 20 characters + null terminator
        std::string errorMessage;

        UsernameMenu(sf::RenderTarget* target);
        static int InputTextCallback(ImGuiInputTextCallbackData *data);
        bool IsValidUsername(const std::string &name);
        void DerivedUpdate();
        ~UsernameMenu();
        void UsernameEntered(std::string username);
        bool CheckForExisting();
    };
}