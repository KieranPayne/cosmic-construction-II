#include "UsernameMenu.hpp"
#include "../imgui/imgui.h"
#include <cctype>
#include <cstring>
#include "SaveManager.hpp"
#include "../Main.hpp"
#include "MainMenu.hpp"
namespace cc
{
	UsernameMenu::UsernameMenu()
	{
		strcpy(username, "");
		errorMessage = "";
    }

	int UsernameMenu::InputTextCallback(ImGuiInputTextCallbackData *data)
	{
		// Reject any character typed that isn't alphanumeric or a space
		if (data->EventChar)
		{
			if (!(std::isalnum((unsigned char)data->EventChar) || data->EventChar == ' '))
			{
				return 1;
			}
		}
		return 0;
	}

	bool UsernameMenu::IsValidUsername(const std::string &name)
	{
		if (name.empty() || name.length() > 20)
		{
			return false;
		}
		for (char c : name)
		{
			if (!(std::isalnum((unsigned char)c) || c == ' '))
			{
				return false;
			}
		}
		return true;
	}

	void UsernameMenu::DerivedUpdate()
	{
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 displaySize = io.DisplaySize;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);

		ImGui::Begin("UsernameMenu", nullptr, windowFlags);

		// Font scaling
		float originalFontScale = ImGui::GetFont()->Scale;
		ImGui::SetWindowFontScale(3.0f);

		const char *title = "Enter Username";
		ImVec2 textSize = ImGui::CalcTextSize(title);
		ImGui::SetCursorPos(ImVec2((displaySize.x - textSize.x) * 0.5f, displaySize.y * 0.2f));
		ImGui::TextUnformatted(title);

		ImGui::SetWindowFontScale(1.0f);

		ImVec2 fieldSize(300, 0);
		ImVec2 windowCenter = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f);

		ImGui::SetCursorPos(ImVec2(windowCenter.x - fieldSize.x * 0.5f, windowCenter.y - 40));
		ImGui::SetNextItemWidth(fieldSize.x);
		ImGui::InputTextWithHint("##Username", "Username", username, IM_ARRAYSIZE(username),
								  ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback);

		ImVec2 buttonSize(150, 50);
		ImGui::SetCursorPos(ImVec2(windowCenter.x - buttonSize.x * 0.5f, windowCenter.y + 10));
		if (ImGui::Button("Continue", buttonSize))
		{
			std::string name = username;
			if (IsValidUsername(name))
			{
				errorMessage = "";
				// TODO: store the username / transition to the next state here
                SaveManager::WriteUsername(name);
                UsernameEntered(name);
			}
			else
			{
				errorMessage = "Username must be 1-20 characters: letters, numbers, or spaces only.";
			}
		}

		if (!errorMessage.empty())
		{
			ImVec2 errSize = ImGui::CalcTextSize(errorMessage.c_str());
			ImGui::SetCursorPos(ImVec2(windowCenter.x - errSize.x * 0.5f, windowCenter.y + 70));
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", errorMessage.c_str());
		}

		ImGui::End();
	}

	UsernameMenu::~UsernameMenu()
	{
	}
    void UsernameMenu::UsernameEntered(std::string username)
    {
        SaveManager::username = username;
        state = std::unique_ptr<Kosmic::State>(new MainMenu());
    }
    bool UsernameMenu::CheckForExisting()
    {
        std::string savedName = SaveManager::GetUsername();
        if (savedName != "" && IsValidUsername(savedName))
        {
            SaveManager::username = savedName;
            return true;
        }
        return false;
    }
}