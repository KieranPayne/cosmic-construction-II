#pragma once
#include "JsonEditor.hpp"
#include "Entity.hpp"
#include "../json.hpp"
#include <SFML/System/Clock.hpp>

namespace cc
{
    class Planet;

    class EntityEditor : public JsonEditor
    {
    public:
        void Draw(Planet *p);

    private:
        bool addingEntity = false;
        Entity *entity = nullptr;

        /// Index of the entity currently being edited, or -1 if none.
        int editingIndex = -1;

        /// Working copy of the entity's JSON so ImGui doesn't snap back to stale state mid-edit.
        nlohmann::json draftJson;

        /// True while `draftJson` has changes that haven't been pushed to `currInputBundle` yet.
        bool draftDirty = false;

        /// True after pushing to `currInputBundle` while waiting for the server tick to return.
        bool awaitingConfirm = false;

        /// Fallback timer so a rejected/overwritten edit doesn't lock the inspector forever.
        sf::Clock confirmClock;

        void QueueEntityChange(Planet *p, int index, const nlohmann::json &j);
    };
}