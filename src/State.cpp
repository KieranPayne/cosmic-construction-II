#include "State.hpp"
#include "Timer.hpp"
namespace Kosmic
{
	State::State()
	{
		// gui = KGUI::GUI();
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
		if (deltaTime == -1){
			//update didnt run
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