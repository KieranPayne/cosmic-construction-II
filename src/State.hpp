#pragma once
#include "Input/Input.hpp"
#include <SFML/Graphics.hpp>
namespace Kosmic
{
	class State
	{
	public:
		sf::RenderTarget *renderTarget;
		InputState inputState;
		double deltaTime = -1;
		void Update(InputState &inputState, double deltaTime);
		virtual void DerivedUpdate();
		void Render();
		virtual void DerivedRender();

		State();
		virtual ~State();
	};
};