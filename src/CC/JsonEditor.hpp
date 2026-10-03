#pragma once
#include "../imgui/imgui.h"
#include "../json.hpp"

#include <string>
#include <unordered_map>
#include <cstdint>
#include "Entity.hpp"
namespace cc
{
    class Planet;

    /**
     * @brief An ImGui window for viewing and editing a planet's entities as json.
     *
     * Each entity is serialized to json and shown as a tree of editable fields. When a field is changed,
     * the json is read back into the entity.
     */
    class JsonEditor
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

        /**
         * @brief Draws one json value (an entity's data) as a tree of editable fields.
         * @param value The json to show. Edited in place.
         * @param label The name shown for the value. Also used as the root of its field paths, so it must be
         *              unique among the values drawn (for example "entity 3").
         * @return true if any field was changed this frame.
         */
        bool DrawEntity(nlohmann::json &value, const char *label)
        {
            return DrawValue(value, label, label);
        }

    private:
        /// True while the editor is building a new entity (adding mode).
        bool addingEntity = false;

        /// The new entity being built in adding mode. Null otherwise. Owned by the editor until "Done" hands it to the planet.
        Entity *entity = nullptr;

        /// Text being typed into each string field, keyed by the field's path. Keeps edits stable between frames.
        std::unordered_map<std::string, std::string> stringBuffers;

        /**
         * @brief Draws one json value, recursing into objects and arrays, with a widget suited to its type.
         * @param value The json value to draw. Edited in place.
         * @param label The name shown next to the value.
         * @param path Where the value is in the json (for example "entity 0.position.x"). Keys `stringBuffers`.
         * @return true if this value, or anything inside it, was changed this frame.
         */
        bool DrawValue(
            nlohmann::json &value,
            const char *label,
            const std::string &path);
    };
}