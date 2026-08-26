#pragma once
#include "../PCH.hpp"
#include "../State.hpp"
#include "Camera.hpp"
#include "Chunk.hpp"
#include "Generator.hpp"
#include "Planet.hpp"

namespace cc
{
	class State : public Kosmic::State
	{
	public:
		Generator generator;
		std::vector<Planet> planets;
		int activePlanet;
		int viewHeight;
		bool paused;
		bool doTick;
		double timeSinceTick;
		double timePerTick;
		void DerivedUpdate();
		void DerivedRender();
		void DisplayPauseMenu();
		State();
		~State();
	};
}