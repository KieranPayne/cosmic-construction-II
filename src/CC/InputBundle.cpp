#include "InputBundle.hpp"

namespace cc
{
    void TileChange::Serialize(Serializer& s)
    {
        s.field("tile",tile);
        s.field("position",position);
        s.field("tileEntity",tileEntity);
    }
    void ChatMessage::Serialize(Serializer& s)
    {
        s.field("origin",origin);
        s.field("message",message);
        s.field("playerUsername",playerUsername);
    }
    InputBundle::InputBundle()
    {

    }
    void InputBundle::Serialize(Serializer& s)
    {
        s.field("username",username);
        s.field("clientId",clientId);
        s.field("playerData",playerData);
        s.field("tilesChanged",tilesChanged);
        s.field("entitiesAdded",entitiesAdded);
        s.field("entitiesChanged",entitiesChanged);
        s.field("changeIndices",changeIndices);
        s.field("chatMessages",chatMessages);
        s.field("knowledgeDatabaseChanged", knowledgeDatabaseChanged);
    }
}