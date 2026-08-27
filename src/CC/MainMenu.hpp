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
			NEW_GAME,
			LOAD_GAME
		} currentState;
		char saveName[100];
		char seed[100];
		std::vector<std::string> names;
		std::vector<std::string> directories;
		std::vector<std::string> playTimes;

		MainMenu();
		void GetNames();
		void DisplayTitleScreen();
		void DisplayNewGame();
		void DisplayLoadGame();
		void DerivedUpdate();
		~MainMenu();
	};
}