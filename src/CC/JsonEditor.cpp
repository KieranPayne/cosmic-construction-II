#include "JsonEditor.hpp"
#include "Planet.hpp"
#include "Serializer.hpp"
namespace cc
{
    void JsonEditor::Draw(Planet *p)
    {
        ImGui::SetNextWindowPos(ImVec2(3,4),ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(315,311),ImGuiCond_Once);
        ImGui::Begin("Entity Editor");
        if (addingEntity)
        {
            if (entity == nullptr)
            {
                entity = new Entity();
            }
            if (ImGui::Button("Done"))
            {
                addingEntity = false;
                p->AddEntity(entity);
                entity = nullptr;
                ImGui::End();
                return;
            }
            ImGui::Separator();
            // JsonWriter jw;
            Serializer sw(Serializer::Mode::WRITE,Serializer::Format::JSON);
            entity->Serialize(sw);
            nlohmann::json j = sw.json();
            // nlohmann::json j = entity->ToJson();
            int currType = j["type"];
            if (DrawEntity(j,"entity"))
            {
                if (j["type"] != currType)
                {
                    delete entity;
                    entity = CreateEntityFromType((Entity::EntityType)j["type"]);
                }else
                {
                    Serializer sr(Serializer::Mode::READ,Serializer::Format::JSON,j);
                    entity->Serialize(sr);
                }
            }
            ImGui::End();
            return;
        }else
        {
            if (ImGui::Button("Add Entity"))
            {
                addingEntity = true;
                ImGui::End();
                return;
            }
            ImGui::Separator();
            for (int i = 0; i < p->entities.size(); i++)
            {
                Serializer sw(Serializer::Mode::WRITE,Serializer::Format::JSON);
                p->entities[i]->Serialize(sw);
                nlohmann::json j = sw.json();
                if (DrawEntity(j, ("entity " + std::to_string(i)).c_str()))
                {
                    Serializer sr(Serializer::Mode::READ,Serializer::Format::JSON,j);
                    p->entities[i]->Serialize(sr);
                }
            }
            ImGui::End();
        }
    }
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
                    std::string childPath =
                        path + "." + it.key();

                    changed |= DrawValue(
                        it.value(),
                        it.key().c_str(),
                        childPath);
                }

                ImGui::TreePop();
            }
        }

        // ARRAY
        else if (value.is_array())
        {
            if (ImGui::TreeNode(
                    label,
                    "%s [%zu]",
                    label,
                    value.size()))
            {
                for (size_t i = 0; i < value.size(); ++i)
                {
                    std::string index =
                        "[" + std::to_string(i) + "]";

                    std::string childPath =
                        path + index;

                    changed |= DrawValue(
                        value[i],
                        index.c_str(),
                        childPath);
                }

                ImGui::TreePop();
            }
        }

        // STRING
        else if (value.is_string())
        {
            auto &buffer = stringBuffers[path];

            // Initialise buffer the first time we see this value
            if (buffer.empty() && !value.get<std::string>().empty())
            {
                buffer = value.get<std::string>();
            }

            // Give ImGui enough room to edit it
            char textBuffer[1024];

            std::strncpy(
                textBuffer,
                buffer.c_str(),
                sizeof(textBuffer) - 1);

            textBuffer[sizeof(textBuffer) - 1] = '\0';

            if (ImGui::InputText(
                    label,
                    textBuffer,
                    sizeof(textBuffer)))
            {
                buffer = textBuffer;
                value = buffer;
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

            if (ImGui::DragScalar(
                    label,
                    ImGuiDataType_S64,
                    &number,
                    0.05f))
            {
                value = number;
                changed = true;
            }
        }

        // UNSIGNED INTEGER
        else if (value.is_number_unsigned())
        {
            uint64_t number = value.get<uint64_t>();

            if (ImGui::DragScalar(
                    label,
                    ImGuiDataType_U64,
                    &number,
                    0.05f))
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