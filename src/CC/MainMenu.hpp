#pragma once
#include "../State.hpp"
namespace cc
{
	/**
	 * @brief The title screen and the menus reached from it: hosting a new or saved game, or joining one.
	 *
	 * Shows one screen at a time, chosen by `currentState`. Escape goes back a screen.
	 */
	class MainMenu : public Kosmic::State
	{
	public:
		/// The screen currently shown.
		enum
		{
			/// Host, Join and Quick Start buttons.
			TITLE_SCREEN,
			/// New Game and Load Game buttons.
			HOST_MENU,
			/// Save name and seed fields for a new save.
			NEW_GAME,
			/// List of existing saves to load or delete.
			LOAD_GAME,
			/// IP address field for joining someone else's game.
			JOIN_GAME
		} currentState;

		/// Name typed for a new save. Emptied when the New Game screen is opened.
		char saveName[100];

		/// Seed text typed for a new save; it is hashed into the numeric seed. Emptied when the New Game screen is opened.
		char seed[100];

		/// IP address typed on the Join screen. Emptied when the Join screen is opened.
		char ipAddress[100];

		/// Message shown in red on the Join screen. Empty when there is nothing to show.
		std::string connectError;

		/// Display names of the saves, newest first. Filled by GetNames(); parallel to `directories` and `playTimes`.
		std::vector<std::string> names;

		/// Folder name of each save inside the save data folder (not a full path). Parallel to `names`.
		std::vector<std::string> directories;

		/// Play time of each save as readable text (for example "1.5 hours"). Parallel to `names`.
		std::vector<std::string> playTimes;

		/**
		 * @brief Creates the menu, starting on the title screen.
		 * @param target The window or texture the menu is drawn to. Handed on to the client when a game starts.
		 */
		MainMenu(sf::RenderTarget *target);

		/// @brief Reloads `names`, `directories` and `playTimes` from the saves on disk, newest first.
		void GetNames();

		/// @brief Draws the title screen: the game's name, then Host, Join and Quick Start buttons.
		void DisplayTitleScreen();

		/// @brief Draws the host menu: New Game, Load Game and Back buttons.
		void DisplayHostMenu();

		/// @brief Draws the new game screen. Done creates the save and starts the game.
		void DisplayNewGame();

		/// @brief Draws the list of saves. Clicking a save loads it; its Delete button removes it.
		void DisplayLoadGame();

		/// @brief Draws the join screen: an IP address field with Connect and Back buttons, and `connectError` if set.
		void DisplayJoinGame();

		/**
		 * @brief Connects to the server at `ipAddress` (port 5000) and replaces this menu with the game client.
		 * @warning Does not yet cope with an invalid address.
		 */
		void ConnectToHost();

		/// @brief Draws the screen picked by `currentState`, then handles Escape (back one screen).
		void DerivedUpdate();

		~MainMenu();
	};
}