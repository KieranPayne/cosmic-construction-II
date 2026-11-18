#include "EntityEditor.hpp"
#include "Planet.hpp"
#include "Serializer.hpp"
#include "CSMessage.hpp"
#include "Client.hpp"

namespace cc
{
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
                p->AddEntity(entity);
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
                    entity = CreateEntityFromType((Entity::EntityType)j["type"]);
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
        else
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
                Serializer sw(Serializer::Mode::WRITE, Serializer::Format::JSON);
                p->entities[i]->Serialize(sw);
                nlohmann::json j = sw.json();
                if (DrawJson(j, ("entity " + std::to_string(i)).c_str()))
                {
                    Serializer sr(Serializer::Mode::READ, Serializer::Format::JSON, j);
                    p->entities[i]->Serialize(sr);
                    sf::Packet packet;
                    packet << (uint16_t)CSMessageType::REQUEST_UPDATE_ENTITIES;
                    packet << (int)1;
                    packet << i;
                    AppendEntityToPacket(packet, p->entities[i].get());
                    p->client->SendPacket(packet);
                }
            }
            ImGui::End();
        }
    }
}