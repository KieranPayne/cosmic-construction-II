#pragma once
#include "../PCH.hpp"
#include "Planet.hpp"
#include "../State.hpp"
#include "PlayerData.hpp"
namespace cc
{
    class Client : public Kosmic::State
    {
        public:
        bool paused = false;
        bool connected = false;
        sf::TcpSocket socket;
        uint64_t id;
        // sf::RenderTarget* target;
        std::vector<PlayerData> otherPlayers;
        std::vector<std::unique_ptr<Planet>> planets;
        int activePlanet;
        void DerivedUpdate();
        void DerivedRender();
        void DisplayPauseMenu();
        Client(sf::RenderTarget* target);
    
        //NETWORKING STUFF
        void ProcessPacket(sf::Packet& packet);
        void SendPacket(sf::Packet& packet);
        void LoadJoinData(sf::Packet& packet);
        void NewPlayerJoined(sf::Packet& packet);
        bool ConnectToServer(sf::IpAddress& ip, unsigned short port);
        bool IsConnected()
        {
            return connected;
        }

        void ReceivePackets();
        void SendUsername();
    };
}