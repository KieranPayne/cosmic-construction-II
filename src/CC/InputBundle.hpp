#pragma once
#include "Serializer.hpp"
#include "PlayerData.hpp"
#include "Tile.hpp"
#include "TileEntity.hpp"
#include "Entity.hpp"
#include "KnowledgeDatabase.hpp"
namespace cc
{
    struct TileChange
    {
        Tile tile;
        sf::Vector2i position;
        std::unique_ptr<TileEntity> tileEntity = std::unique_ptr<TileEntity>(nullptr);
        void Serialize(Serializer& s);
        TileChange() = default;
        TileChange(Tile tile, sf::Vector2i position, std::unique_ptr<TileEntity> tileEntity = nullptr)
            : tile(tile), position(position), tileEntity(std::move(tileEntity)) {}
    };
    enum class ChatMessageOrigin : uint8_t
    {
        /// A message from the server. Shown as "<SERVER> message".
        SERVER,
        /// A chat message from a player. Shown as "<username> message".
        PLAYER
    };
    struct ChatMessage
    {
        ChatMessageOrigin origin = ChatMessageOrigin::SERVER;
        std::string message = "";
        std::string playerUsername = "";
        void Serialize(Serializer& s);
    };
    class InputBundle
    {
        public:
        std::string username = "";
        uint64_t clientId = 0;
        PlayerData playerData;
        std::vector<std::unique_ptr<TileChange>> tilesChanged;
        std::vector<std::unique_ptr<Entity>> entitiesAdded;
        std::vector<std::unique_ptr<Entity>> entitiesChanged;
        std::vector<int> changeIndices;
        std::vector<ChatMessage> chatMessages;
        std::unique_ptr<KnowledgeDatabase> knowledgeDatabaseChanged = nullptr;
        InputBundle();
        void Serialize(Serializer& s);
    };
}