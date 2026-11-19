#include "EntityEditor.hpp"
#include "Planet.hpp"
#include "Serializer.hpp"
#include "CSMessage.hpp"
#include "Client.hpp"

namespace cc
{
    void EntityEditor::QueueEntityChange(Planet *p, int index, const nlohmann::json &j)
    {
        Entity::EntityType type = static_cast<Entity::EntityType>(j["type"].get<uint16_t>());
        Entity *e = CreateEntityFromType(type);
        if (!e)
        {
            return;
        }

        Serializer sr(Serializer::Mode::READ, Serializer::Format::JSON, j);
        e->Serialize(sr);

        auto &b = p->client->currInputBundle;
        for (size_t k = 0; k < b.changeIndices.size(); k++)
        {
            if (b.changeIndices[k] == index)
            {
                b.entitiesChanged[k] = std::unique_ptr<Entity>(e);
                return;
            }
        }

        b.entitiesChanged.push_back(std::unique_ptr<Entity>(e));
        b.changeIndices.push_back(index);
    }

    void EntityEditor::Draw(Planet *p)
    {
        ImGui::SetNextWindowPos(ImVec2(3, 4), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(315, 311), ImGuiCond_Once);
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
                p->client->currInputBundle.entitiesAdded.push_back(std::unique_ptr<Entity>(entity));
                entity = nullptr;
                ImGui::End();
                return;
            }
            ImGui::Separator();
            Serializer sw(Serializer::Mode::WRITE, Serializer::Format::JSON);
            entity->Serialize(sw);
            nlohmann::json j = sw.json();
            int currType = j["type"];
            if (DrawJson(j, "entity"))
            {
                if (j["type"] != currType)
                {
                    delete entity;
                    entity = CreateEntityFromType(static_cast<Entity::EntityType>(j["type"].get<uint16_t>()));
                }
                else
                {
                    Serializer sr(Serializer::Mode::READ, Serializer::Format::JSON, j);
                    entity->Serialize(sr);
                }
            }
            ImGui::End();
            return;
        }

        if (ImGui::Button("Add Entity"))
        {
            addingEntity = true;
            editingIndex = -1;
            draftDirty = false;
            awaitingConfirm = false;
            ImGui::End();
            return;
        }
        ImGui::Separator();

        // Clear draft if the entity was deleted
        if (editingIndex >= static_cast<int>(p->entities.size()))
        {
            editingIndex = -1;
            draftDirty = false;
            awaitingConfirm = false;
        }

        for (int i = 0; i < static_cast<int>(p->entities.size()); i++)
        {
            Serializer sw(Serializer::Mode::WRITE, Serializer::Format::JSON);
            p->entities[i]->Serialize(sw);
            nlohmann::json liveJson = sw.json();

            // Once the authoritative entity matches our draft (or 1s passes), resume live tracking
            if (editingIndex == i && awaitingConfirm && !draftDirty)
            {
                if (liveJson == draftJson || confirmClock.getElapsedTime().asSeconds() > 1.0f)
                {
                    editingIndex = -1;
                    awaitingConfirm = false;
                }
            }

            nlohmann::json &shownJson = (editingIndex == i) ? draftJson : liveJson;

            if (DrawJson(shownJson, ("entity " + std::to_string(i)).c_str()))
            {
                // If the user switched to a different entity while a previous draft was unqueued, flush the old one
                if (editingIndex != -1 && editingIndex != i && draftDirty)
                {
                    QueueEntityChange(p, editingIndex, draftJson);
                }

                // If the entity's "type" changed, rebuild default JSON fields for the new subclass
                if (shownJson["type"] != liveJson["type"] && editingIndex != i)
                {
                    Entity *temp = CreateEntityFromType(static_cast<Entity::EntityType>(shownJson["type"].get<uint16_t>()));
                    if (temp)
                    {
                        temp->position = p->entities[i]->position;
                        temp->size = p->entities[i]->size;
                        Serializer typeSw(Serializer::Mode::WRITE, Serializer::Format::JSON);
                        temp->Serialize(typeSw);
                        shownJson = typeSw.json();
                        delete temp;
                    }
                }

                editingIndex = i;
                draftJson = shownJson;
                draftDirty = true;
                awaitingConfirm = false;
            }

            // Commit to the InputBundle as soon as the user releases the slider/drag or presses Enter
            if (editingIndex == i && draftDirty && !ImGui::IsAnyItemActive())
            {
                QueueEntityChange(p, i, draftJson);
                draftDirty = false;
                awaitingConfirm = true;
                confirmClock.restart();
            }
        }

        ImGui::End();
    }
}