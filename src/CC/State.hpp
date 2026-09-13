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
		std::vector<std::unique_ptr<Planet>> planets;
		int activePlanet;
		int viewHeight;
		uint64_t seed;
		double timeSinceTick;
		double timePerTick;
		bool paused;
		bool doTick;
		void DerivedUpdate();
		void DerivedRender();
		void DisplayPauseMenu();
		void SetSeed(uint64_t seed);
		State();
		~State();
	};
}