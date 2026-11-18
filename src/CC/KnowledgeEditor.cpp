#include "KnowledgeEditor.hpp"
#include "UnlockRequirement.hpp"

#include <cmath>
#include <cstring>

namespace cc
{
    static bool EndsWith(const std::string &s, const std::string &suffix)
    {
        return s.size() >= suffix.size() &&
               s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool KnowledgeEditor::DrawValue(
        nlohmann::json &value,
        const char *label,
        const std::string &path)
    {
        if (value.is_array() && EndsWith(path, ".requirements"))
        {
            return DrawRequirements(value, label);
        }
        if (value.is_object() && EndsWith(path, ".color"))
        {
            return DrawColor(value, label);
        }
        return JsonEditor::DrawValue(value, label, path);
    }

    bool KnowledgeEditor::DrawColor(nlohmann::json &value, const char *label)
    {
        float c[4] = {
            value.value("r", 255) / 255.f,
            value.value("g", 255) / 255.f,
            value.value("b", 255) / 255.f,
            value.value("a", 255) / 255.f};

        if (ImGui::ColorEdit4(label, c))
        {
            value["r"] = static_cast<unsigned>(std::lround(c[0] * 255.f));
            value["g"] = static_cast<unsigned>(std::lround(c[1] * 255.f));
            value["b"] = static_cast<unsigned>(std::lround(c[2] * 255.f));
            value["a"] = static_cast<unsigned>(std::lround(c[3] * 255.f));
            return true;
        }
        return false;
    }

    bool KnowledgeEditor::DrawNodePicker(const char *label, int &index)
    {
        bool changed = false;
        std::string preview = "(none)";
        if (index >= 0 && index < static_cast<int>(nodeNames.size()))
        {
            preview = "[" + std::to_string(index) + "] " + nodeNames[index];
        }

        if (ImGui::BeginCombo(label, preview.c_str()))
        {
            for (int i = 0; i < static_cast<int>(nodeNames.size()); ++i)
            {
                std::string item = "[" + std::to_string(i) + "] " + nodeNames[i];
                if (ImGui::Selectable(item.c_str(), i == index))
                {
                    index = i;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    bool KnowledgeEditor::DrawRequirements(nlohmann::json &value, const char *label)
    {
        bool changed = false;
        const int ITEM = static_cast<int>(UnlockRequirement::Type::ITEM);
        const int CONCEPT = static_cast<int>(UnlockRequirement::Type::CONCEPT);
        const int ACTION = static_cast<int>(UnlockRequirement::Type::ACTION);

        if (ImGui::TreeNode(label, "%s [%zu]", label, value.size()))
        {
            int removeIndex = -1;

            for (size_t i = 0; i < value.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                nlohmann::json &r = value[i];

                // Type dropdown and remove button.
                static const char *typeNames[] = {"Item", "Concept", "Action"};
                int type = r.value("type", ITEM);
                ImGui::SetNextItemWidth(100.f);
                if (ImGui::Combo("##type", &type, typeNames, 3))
                {
                    r["type"] = type;
                    r["amount"] = (type == CONCEPT) ? 0.3f : 1.f;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove"))
                {
                    removeIndex = static_cast<int>(i);
                }

                // Only the fields this type uses.
                if (type == CONCEPT)
                {
                    int node = r.value("node", -1);
                    if (DrawNodePicker("node", node))
                    {
                        r["node"] = node;
                        changed = true;
                    }

                    float amount = r.value("amount", 0.3f);
                    if (ImGui::SliderFloat("min understanding", &amount, 0.f, 1.f))
                    {
                        r["amount"] = amount;
                        changed = true;
                    }
                }
                else
                {
                    char buffer[128];
                    std::string name = r.value("name", std::string());
                    std::strncpy(buffer, name.c_str(), sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    if (ImGui::InputText(type == ACTION ? "action" : "item", buffer, sizeof(buffer)))
                    {
                        r["name"] = std::string(buffer);
                        changed = true;
                    }

                    float amount = r.value("amount", 1.f);
                    if (ImGui::DragFloat(type == ACTION ? "amount" : "quantity", &amount, 0.05f, 0.f, 1000.f))
                    {
                        r["amount"] = amount;
                        changed = true;
                    }
                }

                ImGui::Separator();
                ImGui::PopID();
            }

            if (removeIndex >= 0)
            {
                value.erase(static_cast<size_t>(removeIndex));
                changed = true;
            }

            if (ImGui::Button("Add requirement"))
            {
                value.push_back({{"type", ITEM}, {"name", ""}, {"node", -1}, {"amount", 1.f}});
                changed = true;
            }

            ImGui::TreePop();
        }
        return changed;
    }
}