#include "JsonEditor.hpp"

#include <cstdint>
#include <cstring>

namespace cc
{
    bool JsonEditor::DrawValue(
        nlohmann::json &value,
        const char *label,
        const std::string &path)
    {
        bool changed = false;

        // OBJECT
        if (value.is_object())
        {
            if (ImGui::TreeNode(label))
            {
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    std::string childPath = path + "." + it.key();
                    changed |= DrawValue(it.value(), it.key().c_str(), childPath);
                }
                ImGui::TreePop();
            }
        }

        // ARRAY
        else if (value.is_array())
        {
            if (ImGui::TreeNode(label, "%s [%zu]", label, value.size()))
            {
                for (size_t i = 0; i < value.size(); ++i)
                {
                    std::string index = "[" + std::to_string(i) + "]";
                    changed |= DrawValue(value[i], index.c_str(), path + index);
                }
                ImGui::TreePop();
            }
        }

        // STRING
        else if (value.is_string())
        {
            // Copied from the json every frame, so the field always shows the current value.
            char textBuffer[1024];
            std::strncpy(textBuffer, value.get_ref<const std::string &>().c_str(), sizeof(textBuffer) - 1);
            textBuffer[sizeof(textBuffer) - 1] = '\0';

            if (ImGui::InputText(label, textBuffer, sizeof(textBuffer)))
            {
                value = std::string(textBuffer);
                changed = true;
            }
        }

        // BOOLEAN
        else if (value.is_boolean())
        {
            bool b = value.get<bool>();
            if (ImGui::Checkbox(label, &b))
            {
                value = b;
                changed = true;
            }
        }

        // SIGNED INTEGER
        else if (value.is_number_integer())
        {
            int64_t number = value.get<int64_t>();
            if (ImGui::DragScalar(label, ImGuiDataType_S64, &number, 0.05f))
            {
                value = number;
                changed = true;
            }
        }

        // UNSIGNED INTEGER
        else if (value.is_number_unsigned())
        {
            uint64_t number = value.get<uint64_t>();
            if (ImGui::DragScalar(label, ImGuiDataType_U64, &number, 0.05f))
            {
                value = number;
                changed = true;
            }
        }

        // FLOAT
        else if (value.is_number_float())
        {
            float number = value.get<double>();
            if (ImGui::DragFloat(label, &number, 0.05f))
            {
                value = number;
                changed = true;
            }
        }

        // NULL
        else if (value.is_null())
        {
            ImGui::Text("%s: null", label);
        }

        return changed;
    }
}