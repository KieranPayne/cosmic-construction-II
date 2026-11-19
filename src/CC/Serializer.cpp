#include "Serializer.hpp"
#include "Entity.hpp"

namespace cc
{
    void Serializer::field(const std::string name, std::unique_ptr<Entity> &value)
    {
        if (format == Format::JSON)
        {
            if (mode == Mode::WRITE)
            {
                if (!value)
                {
                    j_[name] = nullptr;
                }
                else
                {
                    Serializer sub(mode, format);
                    value->Serialize(sub);
                    j_[name] = sub.json();
                }
            }
            else // READ
            {
                if (!j_.contains(name) || j_.at(name).is_null())
                {
                    value.reset();
                    return;
                }
                const auto &obj = j_.at(name);
                auto type = static_cast<Entity::EntityType>(obj.value("type", 0));
                value.reset(CreateEntityFromType(type));
                if (value)
                {
                    Serializer sub(mode, format, obj);
                    value->Serialize(sub);
                }
            }
        }
        else // BINARY
        {
            if (mode == Mode::WRITE)
            {
                bool hasValue = (value != nullptr);
                writeRaw(hasValue);
                if (hasValue)
                {
                    value->Serialize(*this);
                }
            }
            else // READ
            {
                bool hasValue = false;
                readRaw(hasValue);
                if (!hasValue)
                {
                    value.reset();
                    return;
                }

                uint16_t rawType = 0;
                readRaw(rawType);
                readPos_ -= sizeof(uint16_t);

                value.reset(CreateEntityFromType(static_cast<Entity::EntityType>(rawType)));
                if (value)
                {
                    value->Serialize(*this);
                }
                else
                {
                    throw std::runtime_error("Serializer: unknown EntityType " + std::to_string(rawType));
                }
            }
        }
    }

    void Serializer::field(const std::string name, std::vector<std::unique_ptr<Entity>> &value)
    {
        if (format == Format::JSON)
        {
            if (mode == Mode::WRITE)
            {
                nlohmann::json arr = nlohmann::json::array();
                for (auto &elem : value)
                {
                    if (!elem)
                    {
                        arr.push_back(nullptr);
                        continue;
                    }
                    Serializer sub(mode, format);
                    elem->Serialize(sub);
                    arr.push_back(sub.json());
                }
                j_[name] = arr;
            }
            else // READ
            {
                if (!j_.contains(name))
                    return;
                value.clear();
                for (auto &elem_json : j_.at(name))
                {
                    if (elem_json.is_null())
                    {
                        value.push_back(nullptr);
                        continue;
                    }
                    auto type = static_cast<Entity::EntityType>(elem_json.value("type", 0));
                    std::unique_ptr<Entity> elem(CreateEntityFromType(type));
                    if (elem)
                    {
                        Serializer sub(mode, format, elem_json);
                        elem->Serialize(sub);
                    }
                    value.push_back(std::move(elem));
                }
            }
        }
        else // BINARY
        {
            if (mode == Mode::WRITE)
            {
                uint32_t count = static_cast<uint32_t>(value.size());
                writeRaw(count);
                for (auto &elem : value)
                {
                    bool hasValue = (elem != nullptr);
                    writeRaw(hasValue);
                    if (hasValue)
                        elem->Serialize(*this);
                }
            }
            else // READ
            {
                uint32_t count = 0;
                readRaw(count);
                value.clear();
                value.reserve(count);
                for (uint32_t i = 0; i < count; i++)
                {
                    bool hasValue = false;
                    readRaw(hasValue);
                    if (!hasValue)
                    {
                        value.push_back(nullptr);
                        continue;
                    }

                    uint16_t rawType = 0;
                    readRaw(rawType);
                    readPos_ -= sizeof(uint16_t);

                    std::unique_ptr<Entity> elem(CreateEntityFromType(static_cast<Entity::EntityType>(rawType)));
                    if (!elem)
                        throw std::runtime_error("Serializer: unknown EntityType " + std::to_string(rawType));

                    elem->Serialize(*this);
                    value.push_back(std::move(elem));
                }
            }
        }
    }
}