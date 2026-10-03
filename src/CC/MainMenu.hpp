#pragma once
#include "../State.hpp"
namespace cc
{
	class MainMenu : public Kosmic::State
	{
	public:
		enum
		{
			TITLE_SCREEN,
			HOST_MENU,
			NEW_GAME,
			LOAD_GAME,
			JOIN_GAME
		} currentState;
		char saveName[100];
		char seed[100];
		char ipAddress[100];
		std::string connectError;
		std::vector<std::string> names;
		std::vector<std::string> directories;
		std::vector<std::string> playTimes;

		MainMenu(sf::RenderTarget* target);
		void GetNames();
		void DisplayTitleScreen();
		void DisplayHostMenu();
		void DisplayNewGame();
		void DisplayLoadGame();
		void DisplayJoinGame();
		void ConnectToHost();
		void DerivedUpdate();
		~MainMenu();
	};
}