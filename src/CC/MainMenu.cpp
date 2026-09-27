#include "MainMenu.hpp"
#include "../Main.hpp"
#include "../Timer.hpp"
#include "../imgui/imgui.h"
#include "SaveManager.hpp"
#include "Utils.hpp"
#include "Client.hpp"
namespace cc
{
	MainMenu::MainMenu()
	{
		currentState = TITLE_SCREEN;
		strcpy(ipAddress, "");
		connectError = "";
	}

	void MainMenu::DerivedUpdate()
	{
		if (currentState == TITLE_SCREEN)
		{
			DisplayTitleScreen();
		}
		else if (currentState == HOST_MENU)
		{
			DisplayHostMenu();
		}
		else if (currentState == NEW_GAME)
		{
			DisplayNewGame();
		}
		else if (currentState == JOIN_GAME)
		{
			DisplayJoinGame();
		}
		else
		{
			DisplayLoadGame();
		}

		if (inputState.Pressed(sf::Keyboard::Key::Escape) && (currentState != TITLE_SCREEN))
		{
			if (currentState == NEW_GAME || currentState == LOAD_GAME)
			{
				currentState = HOST_MENU;
			}
			else
			{
				currentState = TITLE_SCREEN;
			}
		}
	}
	void MainMenu::DisplayTitleScreen()
	{
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 displaySize = io.DisplaySize;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);

		ImGui::Begin("MainMenu", nullptr, windowFlags);

		// Font scaling
		float originalFontScale = ImGui::GetFont()->Scale;
		ImGui::SetWindowFontScale(3.0f); // Double font size

		const char *title = "Cosmic Construction II";
		ImVec2 textSize = ImGui::CalcTextSize(title);
		ImGui::SetCursorPos(ImVec2((displaySize.x - textSize.x) * 0.5f, displaySize.y * 0.2f));
		ImGui::TextUnformatted(title);

		ImGui::SetWindowFontScale(1.0f); // Reset font scale

		// Buttons
		ImVec2 buttonSize(200, 50);
		ImVec2 windowCenter = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f);

		ImGui::SetCursorPos(ImVec2(windowCenter.x - buttonSize.x * 0.5f, windowCenter.y - buttonSize.y - 10));
		if (ImGui::Button("Host", buttonSize))
		{
			currentState = HOST_MENU;
		}

		ImGui::SetCursorPos(ImVec2(windowCenter.x - buttonSize.x * 0.5f, windowCenter.y + 10));
		if (ImGui::Button("Join", buttonSize))
		{
			strcpy(ipAddress, "");
			connectError = "";
			currentState = JOIN_GAME;
		}

		ImGui::SetCursorPos(ImVec2(windowCenter.x - buttonSize.x * 0.5f, windowCenter.y + 20 + buttonSize.y));
		if (ImGui::Button("Quick Start", buttonSize))
		{
			SaveManager::CreateSave("quick start save", "");
		}

		ImGui::End();
	}
	void MainMenu::DisplayHostMenu()
	{
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 displaySize = io.DisplaySize;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);

		ImGui::Begin("HostMenu", nullptr, windowFlags);

		float originalFontScale = ImGui::GetFont()->Scale;
		ImGui::SetWindowFontScale(3.0f);

		const char *title = "Host Game";
		ImVec2 textSize = ImGui::CalcTextSize(title);
		ImGui::SetCursorPos(ImVec2((displaySize.x - textSize.x) * 0.5f, displaySize.y * 0.2f));
		ImGui::TextUnformatted(title);

		ImGui::SetWindowFontScale(1.0f);

		ImVec2 buttonSize(200, 50);
		ImVec2 windowCenter = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f);

		ImGui::SetCursorPos(ImVec2(windowCenter.x - buttonSize.x * 0.5f, windowCenter.y - buttonSize.y - 10));
		if (ImGui::Button("New Game", buttonSize))
		{
			strcpy(saveName, "");
			strcpy(seed, "");
			currentState = NEW_GAME;
		}

		ImGui::SetCursorPos(ImVec2(windowCenter.x - buttonSize.x * 0.5f, windowCenter.y + 10));
		if (ImGui::Button("Load Game", buttonSize))
		{
			GetNames();
			currentState = LOAD_GAME;
		}

		ImGui::SetCursorPos(ImVec2(20, displaySize.y - 70));
		if (ImGui::Button("Back", ImVec2(100, 40)))
		{
			currentState = TITLE_SCREEN;
		}

		ImGui::End();
	}
	void MainMenu::DisplayNewGame()
	{
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 displaySize = io.DisplaySize;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);

		ImGui::Begin("NewGameMenu", nullptr, windowFlags);
		float centerX = displaySize.x * 0.5f;
		ImGui::Text("New Game");
		ImGui::InputTextWithHint("##SaveName", "New Save", saveName, IM_ARRAYSIZE(saveName));
		ImGui::Text("Seed:");
		ImGui::InputText("##Seed", seed, IM_ARRAYSIZE(seed));
		if (ImGui::Button("Done"))
		{
			SaveManager::CreateSave(saveName, seed);
		}
		ImGui::SameLine();
		if (ImGui::Button("Back"))
		{
			currentState = HOST_MENU;
		}
		ImGui::End();
	}
	void MainMenu::GetNames()
	{
		// directories = {};
		playTimes = {};
		names = {};
		directories = {};
		std::vector<uint64_t> times = {}; 
		std::string path = SaveManager::GetSavedataDir();
		if (SaveManager::DirExists(path))
		{
			auto dirs = SaveManager::ListDirectories(path);
			for (auto d : dirs)
			{
				std::string fullPath = path + "/" + d;
				nlohmann::json j = nlohmann::json::parse(SaveManager::ReadData(fullPath + "/metadata.json"));
				uint64_t time = j["modified"];
				std::string playTime;
				int seconds = j["playTime"];
				if (seconds < 60){
					playTime = std::to_string(seconds) + " seconds";
				}else if (seconds < 60 * 60){
					playTime =  (std::ostringstream() << std::fixed << std::setprecision(1) << (seconds / 60.f)).str() + " minutes";
				}else if (seconds < 60 * 60 * 24){
					playTime =  (std::ostringstream() << std::fixed << std::setprecision(1) << (seconds / 60.f / 60.f)).str() + " hours";
				}else{
					playTime =  (std::ostringstream() << std::fixed << std::setprecision(1) << (seconds / 60.f / 60.f / 24.f)).str() + " days";
				}
				bool found = false;
				for (int i = 0; i < times.size(); i ++){
					if (times[i] < time){
						times.insert(times.begin() + i,time);
						names.insert(names.begin() + i, j["saveName"]);
						playTimes.insert(playTimes.begin() + i,playTime);
						directories.insert(directories.begin() + i,d);
						found = true;
						break;
					}
				}
				if (!found){
					times.push_back(time);
					names.push_back(j["saveName"]);
					directories.push_back(d);
					playTimes.push_back(playTime);
				} 
				// auto metadata = Split(SaveManager::ReadData(fullPath + "/metadata.txt"), '\n');
				// names.push_back(metadata[0]);
				// directories.push_back(fullPath);
			}
		}
	}
	void MainMenu::DisplayLoadGame()
	{
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 displaySize = io.DisplaySize;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);

		ImGui::Begin("LoadGameMenu", nullptr, windowFlags);
		ImGui::Text("Load Game");
		ImGui::BeginChild("BlockList", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
		int dirToDelete = -1;
		for (int i = 0; i < names.size(); ++i)
		{
			ImGui::PushID(i); // Ensure unique IDs per block

			// Draw visible box around block info
			ImGui::BeginGroup();
			ImGui::BeginChild("BlockBox", ImVec2(0, 60), true); // Box with fixed height

			ImGui::Text("%s", names[i].c_str());

			ImGui::SameLine();
			
			ImGui::Text("%s", ("            play time: " + playTimes[i]).c_str());

			ImGui::SameLine();


			if (ImGui::Button("Delete"))
			{
				dirToDelete = i;
			}

			ImGui::EndChild();
			if (ImGui::IsItemClicked())
			{
				auto dirs = SaveManager::ListDirectories(SaveManager::GetSavedataDir());
				int index = 0;
				for (int j = 0; j < dirs.size(); j ++){
					if (dirs[j] == directories[i]){
						index = j;
						break;
					}
				}
				SaveManager::LoadServer(index);
			}
			ImGui::EndGroup();

			ImGui::Spacing();
			ImGui::PopID();
		}

		if (dirToDelete != -1)
		{
			SaveManager::DeleteDirectory(directories[dirToDelete]);
			GetNames();
		}

		ImGui::EndChild();
		ImGui::End();
	}
	void MainMenu::DisplayJoinGame()
	{
		ImGuiIO &io = ImGui::GetIO();
		ImVec2 displaySize = io.DisplaySize;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(displaySize);

		ImGui::Begin("JoinMenu", nullptr, windowFlags);

		float originalFontScale = ImGui::GetFont()->Scale;
		ImGui::SetWindowFontScale(3.0f);

		const char *title = "Join Game";
		ImVec2 textSize = ImGui::CalcTextSize(title);
		ImGui::SetCursorPos(ImVec2((displaySize.x - textSize.x) * 0.5f, displaySize.y * 0.2f));
		ImGui::TextUnformatted(title);

		ImGui::SetWindowFontScale(1.0f);

		ImVec2 fieldSize(300, 0);
		ImVec2 windowCenter = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f);

		ImGui::SetCursorPos(ImVec2(windowCenter.x - fieldSize.x * 0.5f, windowCenter.y - 40));
		ImGui::SetNextItemWidth(fieldSize.x);
		ImGui::InputTextWithHint("##IpAddress", "IP Address (e.g. 127.0.0.1)", ipAddress, IM_ARRAYSIZE(ipAddress));

		ImVec2 buttonSize(150, 50);
		ImGui::SetCursorPos(ImVec2(windowCenter.x - buttonSize.x - 10, windowCenter.y + 10));
		if (ImGui::Button("Connect", buttonSize))
		{
			ConnectToHost();
		}

		ImGui::SetCursorPos(ImVec2(windowCenter.x + 10, windowCenter.y + 10));
		if (ImGui::Button("Back", buttonSize))
		{
			currentState = TITLE_SCREEN;
		}

		if (!connectError.empty())
		{
			ImVec2 errSize = ImGui::CalcTextSize(connectError.c_str());
			ImGui::SetCursorPos(ImVec2(windowCenter.x - errSize.x * 0.5f, windowCenter.y + 70));
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", connectError.c_str());
		}

		ImGui::End();
	}
	void MainMenu::ConnectToHost()
	{
		auto ipResult = sf::IpAddress::resolve(ipAddress);
		//TODO: deal with case where not valid ip address
		Client* c = new Client(state->renderTarget);
		c->ConnectToServer(ipResult.value(),5000);
		state = std::unique_ptr<Kosmic::State>(c);
	}

	MainMenu::~MainMenu()
	{
	}
}