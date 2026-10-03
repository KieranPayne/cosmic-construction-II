#pragma once
#include "Input/Input.hpp"
#include <SFML/Graphics.hpp>
namespace Kosmic
{
	/**
	 * @brief A screen of the program (a menu, the game, ...). Only one is active at a time.
	 *
	 * The main loop calls Update() then Render() on the active state every frame. Derived classes fill in
	 * DerivedUpdate() and DerivedRender().
	 */
	class State
	{
	public:
		/// Where this state draws. Has no initial value; derived classes set it.
		sf::RenderTarget *renderTarget;

		/// The input from the current frame. Set by Update().
		InputState inputState;

		/// Seconds the last frame took. -1 until Update() has run for the first time.
		double deltaTime = -1;

		/**
		 * @brief Runs one frame of the state: stores the input and frame time, then calls DerivedUpdate().
		 * @param inputState The input for this frame.
		 * @param deltaTime Seconds since the previous frame.
		 */
		void Update(InputState &inputState, double deltaTime);

		/// @brief The state's own per-frame logic. Does nothing by default; override it.
		virtual void DerivedUpdate();

		/// @brief Draws the state by calling DerivedRender(). Does nothing if Update() hasn't run yet.
		void Render();

		/// @brief The state's own drawing. Does nothing by default; override it.
		virtual void DerivedRender();

		State();
		virtual ~State();
	};
}