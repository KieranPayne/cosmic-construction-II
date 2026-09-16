#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
#include "../State.hpp"
namespace cc
{
    class Client : public Kosmic::State
    {
        public:
        bool paused = false;
        // sf::RenderTarget* target;
        std::vector<std::unique_ptr<Planet>> planets;
        int activePlanet;
        void DerivedUpdate();
        void DerivedRender();
        void DisplayPauseMenu();
        Client(sf::RenderTarget* target);
    };
}