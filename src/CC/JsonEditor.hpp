#pragma once
#include "../imgui/imgui.h"
#include "../json.hpp"

#include <string>

namespace cc
{
    /**
     * @brief A general-purpose ImGui editor for json values.
     *
     * Shows any json value as a tree of editable fields. It knows nothing about game objects: build a
     * widget for a specific type by serializing it to json, calling DrawJson, and reading the json back
     * if it returns true (see EntityEditor).
     *
     * Derive and override DrawValue to give particular fields custom widgets, falling back to
     * JsonEditor::DrawValue for everything else.
     */
    class JsonEditor
    {
    public:
        virtual ~JsonEditor() = default;

        /**
         * @brief Draws a json value as a tree of editable fields.
         * @param value The json to show. Edited in place.
         * @param label The name shown for the value. Also the root of its field paths, so it should be
         *              unique among the values drawn in the same window (for example "entity 3").
         * @param openRoot Opens the root tree node this call. Pass true only on the first frame a
         *                 window is shown, since it overrides the user collapsing the node.
         * @return true if any field was changed this frame.
         */
        bool DrawJson(nlohmann::json &value, const char *label, bool openRoot = false)
        {
            if (openRoot && (value.is_object() || value.is_array()))
            {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            }
            return DrawValue(value, label, label);
        }

    protected:
        /**
         * @brief Draws one json value, recursing into objects and arrays, with a widget suited to its type.
         * @param value The json value to draw. Edited in place.
         * @param label The name shown next to the value.
         * @param path Where the value is in the json (for example "entity 0.position.x").
         *             Useful in overrides to recognise specific fields.
         * @return true if this value, or anything inside it, was changed this frame.
         */
        virtual bool DrawValue(
            nlohmann::json &value,
            const char *label,
            const std::string &path);
    };
}