#pragma once
#include "JsonEditor.hpp"
#include "Entity.hpp"

namespace cc
{
    class Planet;

    /**
     * @brief An ImGui window for viewing and editing a planet's entities as json.
     *
     * Each entity is serialized to json and shown with JsonEditor. When a field is changed, the json is
     * read back into the entity.
     */
    class EntityEditor : public JsonEditor
    {
    public:
        /**
         * @brief Draws the "Entity Editor" window. Call once per frame.
         *
         * Has two modes:
         *  - Normal: lists every entity on the planet. Editing one applies the change locally and sends a
         *    REQUEST_UPDATE_ENTITIES packet to the server.
         *  - Adding: edits a new entity that is not on the planet yet. Changing its "type" replaces it with a
         *    new entity of that type. "Done" adds it to the planet.
         * @param p The planet whose entities are edited. Needs a connected `client` for edits to reach the server.
         */
        void Draw(Planet *p);

    private:
        /// True while the editor is building a new entity (adding mode).
        bool addingEntity = false;

        /// The new entity being built in adding mode. Null otherwise. Owned by the editor until "Done" hands it to the planet.
        Entity *entity = nullptr;
    };
}