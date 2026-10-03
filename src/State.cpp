#include "State.hpp"
#include "Timer.hpp"
namespace Kosmic
{
	State::State()
	{
	}
	void State::Update(InputState &inputState, double deltaTime)
	{
		this->inputState = inputState;
		this->deltaTime = deltaTime;
		DerivedUpdate();
	}
	void State::DerivedUpdate()
	{
	}
	void State::Render()
	{
		// Update() hasn't run yet, so there is nothing to draw
		if (deltaTime == -1)
		{
			return;
		}
		DerivedRender();
	}
	void State::DerivedRender()
	{
	}
	State::~State()
	{
	}
}