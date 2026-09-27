#pragma once
#include "../json.hpp"
#include "../PCH.hpp"
namespace cc
{
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

        // Scalar fields (int, float, std::string, bool...)
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
                        uint32_t len;
                        readRaw(len);
                        value.assign(reinterpret_cast<const char *>(&b_[readPos_]), len);
                        readPos_ += len;
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

        // specific cases for vectors (cant add a serialize function to them)
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
                    const auto &j = j_.at(name);
                    value.x = j.at("x").get<T>();
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
                    const auto &j = j_.at(name);
                    value.x = j.at("x").get<T>();
                    value.y = j.at("y").get<T>();
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

        // Custom types with their own Serialize()
        template <typename T>
        std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<T, std::string>>
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
                    Serializer sub(mode, format, j_.at(name));
                    value.Serialize(sub);
                }
            }
            else // BINARY — no keys, so just recurse into *this*, sharing the buffer/cursor
            {
                value.Serialize(*this);
            }
        }

        // pointers to custom types with their own Serialize()
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

        // Vectors of primitives — nlohmann::json handles this directly
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
                        field(name, elem); // reuses the scalar overload above per-element
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

        // Vectors of custom serializable types — loop + recurse
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

        nlohmann::json json() const { return j_; }
        std::vector<uint8_t> binary() const { return b_; }

    private:
        nlohmann::json j_;
        std::vector<uint8_t> b_;
        size_t readPos_ = 0; // cursor for binary reads

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