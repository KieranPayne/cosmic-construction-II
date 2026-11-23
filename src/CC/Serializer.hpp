#pragma once
#include "../json.hpp"
#include "../PCH.hpp"

namespace cc
{
    class Entity;
    class Action;

    /**
     * @brief Saves or loads an object's fields, to json or to compact binary, using one piece of code for both directions.
     *
     * A class describes itself once, in a `Serialize(Serializer &s)` function that calls `s.field(name, value)` for
     * each member. If the serializer is in WRITE mode each call stores the value; in READ mode each call fills the
     * value in. That keeps saving and loading from drifting apart.
     *
     * In JSON format fields are stored under their names, and are looked up by name when reading.
     * In BINARY format the names are ignored and the values are stored one after another, so the fields must be
     * read in exactly the same order and with the same types as they were written.
     */
    class Serializer
    {
    public:
        enum class Mode
        {
            READ,
            WRITE
        };

        Mode mode;

        enum class Format
        {
            JSON,
            BINARY
        };

        Format format;

        Serializer(Mode mode, Format format, nlohmann::json j = {}, std::vector<uint8_t> b = {})
        {
            this->mode = mode;
            this->format = format;
            j_ = j;
            b_ = b;
        }

        template <typename T>
        std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, std::string>>
        field(const std::string name, T &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    j_[name] = value;
                }
                else
                {
                    if (j_.contains(name))
                        value = j_.at(name).get<T>();
                }
            }
            else // BINARY
            {
                if constexpr (std::is_same_v<T, std::string>)
                {
                    if (mode == Mode::WRITE)
                    {
                        uint32_t len = static_cast<uint32_t>(value.size());
                        writeRaw(len);
                        b_.insert(b_.end(), value.begin(), value.end());
                    }
                    else
                    {
                        uint32_t len = 0;
                        readRaw(len);
                        if (len == 0)
                        {
                            value.clear();
                        }
                        else
                        {
                            if (readPos_ + len > b_.size())
                                throw std::runtime_error("Serializer: unexpected end of binary buffer");
                            value.assign(reinterpret_cast<const char *>(b_.data() + readPos_), len);
                            readPos_ += len;
                        }
                    }
                }
                else
                {
                    if (mode == Mode::WRITE)
                        writeRaw(value);
                    else
                        readRaw(value);
                }
            }
        }

        template <typename T>
        void field(const char *name, sf::Vector2<T> &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    nlohmann::json j;
                    j["x"] = value.x;
                    j["y"] = value.y;
                    j_[name] = j;
                }
                else
                {
                    if (!j_.contains(name))
                        return;
                    const auto &j = j_.at(name);
                    if (j.contains("x"))
                        value.x = j.at("x").get<T>();
                    if (j.contains("y"))
                        value.y = j.at("y").get<T>();
                }
            }
            else // BINARY
            {
                if (mode == Mode::WRITE)
                {
                    writeRaw(value.x);
                    writeRaw(value.y);
                }
                else
                {
                    readRaw(value.x);
                    readRaw(value.y);
                }
            }
        }

        template <typename T>
        void field(const char *name, sf::Vector3<T> &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    nlohmann::json j;
                    j["x"] = value.x;
                    j["y"] = value.y;
                    j["z"] = value.z;
                    j_[name] = j;
                }
                else
                {
                    if (!j_.contains(name))
                        return;
                    const auto &j = j_.at(name);
                    if (j.contains("x"))
                        value.x = j.at("x").get<T>();
                    if (j.contains("y"))
                        value.y = j.at("y").get<T>();
                    if (j.contains("z"))
                        value.z = j.at("z").get<T>();
                }
            }
            else // BINARY
            {
                if (mode == Mode::WRITE)
                {
                    writeRaw(value.x);
                    writeRaw(value.y);
                    writeRaw(value.z);
                }
                else
                {
                    readRaw(value.x);
                    readRaw(value.y);
                    readRaw(value.z);
                }
            }
        }

        template <typename T>
        std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_enum_v<T> && !std::is_same_v<T, std::string>>
        field(const std::string name, T &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    Serializer sub(mode, format);
                    value.Serialize(sub);
                    j_[name] = sub.json();
                }
                else
                {
                    if (!j_.contains(name))
                        return;
                    Serializer sub(mode, format, j_.at(name));
                    value.Serialize(sub);
                }
            }
            else // BINARY
            {
                value.Serialize(*this);
            }
        }

        template <typename T>
        std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<T, std::string>>
        field(const std::string name, T *value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    Serializer sub(mode, format);
                    value->Serialize(sub);
                    j_[name] = sub.json();
                }
                else
                {
                    if (!j_.contains(name))
                        return;
                    if (!value)
                        value = new T();
                    Serializer sub(mode, format, j_.at(name));
                    value->Serialize(sub);
                }
            }
            else // BINARY
            {
                if (mode == Mode::WRITE)
                {
                    value->Serialize(*this);
                }
                else
                {
                    if (!value)
                        value = new T();
                    value->Serialize(*this);
                }
            }
        }

        template <typename T>
        std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, std::string>>
        field(const std::string name, std::vector<T> &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    j_[name] = value;
                }
                else
                {
                    if (j_.contains(name))
                        value = j_.at(name).get<std::vector<T>>();
                }
            }
            else // BINARY
            {
                if (mode == Mode::WRITE)
                {
                    uint32_t count = static_cast<uint32_t>(value.size());
                    writeRaw(count);
                    for (auto &elem : value)
                        field(name, elem);
                }
                else
                {
                    uint32_t count;
                    readRaw(count);
                    value.resize(count);
                    for (auto &elem : value)
                        field(name, elem);
                }
            }
        }

        template <typename T>
        std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<T, std::string>>
        field(const std::string name, std::vector<T> &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    nlohmann::json arr = nlohmann::json::array();
                    for (auto &elem : value)
                    {
                        Serializer sub(mode, format);
                        elem.Serialize(sub);
                        arr.push_back(sub.json());
                    }
                    j_[name] = arr;
                }
                else
                {
                    if (!j_.contains(name))
                        return;
                    value.clear();
                    for (auto &elem_json : j_.at(name))
                    {
                        T elem{};
                        Serializer sub(mode, format, elem_json);
                        elem.Serialize(sub);
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
                        elem.Serialize(*this);
                }
                else
                {
                    uint32_t count;
                    readRaw(count);
                    value.clear();
                    value.reserve(count);
                    for (uint32_t i = 0; i < count; i++)
                    {
                        T elem{};
                        elem.Serialize(*this);
                        value.push_back(std::move(elem));
                    }
                }
            }
        }

        template <typename T>
        std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<T, std::string>>
        field(const std::string name, std::vector<std::unique_ptr<T>> &value)
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
                else
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
                        auto elem = std::make_unique<T>();
                        Serializer sub(mode, format, elem_json);
                        elem->Serialize(sub);
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
                else
                {
                    uint32_t count;
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
                        auto elem = std::make_unique<T>();
                        elem->Serialize(*this);
                        value.push_back(std::move(elem));
                    }
                }
            }
        }

        void field(const std::string name, std::size_t &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    j_[name] = value;
                }
                else
                {
                    if (j_.contains(name))
                        value = j_.at(name).get<std::size_t>();
                }
            }
            else // BINARY
            {
                if (mode == Mode::WRITE)
                    writeRaw(value);
                else
                    readRaw(value);
            }
        }

        template <typename T>
        void field(const std::string name, std::unique_ptr<T> &value)
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
                    if (!value)
                    {
                        value = std::make_unique<T>();
                    }
                    Serializer sub(mode, format, j_.at(name));
                    value->Serialize(sub);
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
                    if (!value)
                    {
                        value = std::make_unique<T>();
                    }
                    value->Serialize(*this);
                }
            }
        }

        template <typename T>
        std::enable_if_t<std::is_enum_v<T>>
        field(const std::string name, T &value)
        {
            using Underlying = std::underlying_type_t<T>;
            Underlying raw = static_cast<Underlying>(value);
            field(name, raw);
            if (mode == Mode::READ)
            {
                value = static_cast<T>(raw);
            }
        }

        void field(const std::string name, sf::Color &value)
        {
            if (format == Format::JSON)
            {
                if (mode == Mode::WRITE)
                {
                    nlohmann::json j;
                    j["r"] = static_cast<unsigned>(value.r);
                    j["g"] = static_cast<unsigned>(value.g);
                    j["b"] = static_cast<unsigned>(value.b);
                    j["a"] = static_cast<unsigned>(value.a);
                    j_[name] = j;
                }
                else
                {
                    if (!j_.contains(name))
                        return;
                    const auto &j = j_.at(name);
                    if (j.contains("r"))
                        value.r = static_cast<uint8_t>(j.at("r").get<unsigned>());
                    if (j.contains("g"))
                        value.g = static_cast<uint8_t>(j.at("g").get<unsigned>());
                    if (j.contains("b"))
                        value.b = static_cast<uint8_t>(j.at("b").get<unsigned>());
                    if (j.contains("a"))
                        value.a = static_cast<uint8_t>(j.at("a").get<unsigned>());
                }
            }
            else // BINARY
            {
                if (mode == Mode::WRITE)
                {
                    writeRaw(value.r);
                    writeRaw(value.g);
                    writeRaw(value.b);
                    writeRaw(value.a);
                }
                else
                {
                    readRaw(value.r);
                    readRaw(value.g);
                    readRaw(value.b);
                    readRaw(value.a);
                }
            }
        }

        // Implemented in Serializer.cpp to avoid circular include with Entity.hpp
        void field(const std::string name, std::unique_ptr<Entity> &value);
        void field(const std::string name, std::vector<std::unique_ptr<Entity>> &value);
        void field(const std::string name, std::unique_ptr<Action> &value);
        void field(const std::string name, std::vector<std::unique_ptr<Action>> &value);

        nlohmann::json json() const { return j_; }
        std::vector<uint8_t> binary() const { return b_; }

    private:
        nlohmann::json j_;
        std::vector<uint8_t> b_;
        size_t readPos_ = 0;

        template <typename T>
        void writeRaw(const T &value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "writeRaw requires a trivially copyable type");
            const uint8_t *p = reinterpret_cast<const uint8_t *>(&value);
            b_.insert(b_.end(), p, p + sizeof(T));
        }

        template <typename T>
        void readRaw(T &value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "readRaw requires a trivially copyable type");
            std::memcpy(&value, b_.data() + readPos_, sizeof(T));
            readPos_ += sizeof(T);
        }
    };
}