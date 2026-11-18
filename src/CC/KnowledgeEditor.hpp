#pragma once
#include "JsonEditor.hpp"
#include <string>
#include <vector>

namespace cc
{
    /**
     * @brief A JsonEditor with custom widgets for the knowledge graph's data.
     *
     * - `requirements` lists get add/remove buttons, a type dropdown, and only the fields that type uses.
     * - `color` gets a colour picker.
     * Everything else uses the generic widgets, so new fields still appear without changes here.
     */
    class KnowledgeEditor : public JsonEditor
    {
    public:
        /// Name of every node in the graph, by index. The graph refreshes this before drawing; used by node dropdowns.
        std::vector<std::string> nodeNames;

    protected:
        bool DrawValue(
            nlohmann::json &value,
            const char *label,
            const std::string &path) override;

    private:
        bool DrawRequirements(nlohmann::json &value, const char *label);
        bool DrawColor(nlohmann::json &value, const char *label);
        bool DrawNodePicker(const char *label, int &index);
    };
}