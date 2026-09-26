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
        field(const std::string name,T &value)
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
            }
            else
            {
                if (!value)
                    value = new T();
                Serializer sub(mode,format,j_.at(name));
                value->Serialize(sub);
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
                    j_[name] = value; // nlohmann already knows how to dump vector<int>, vector<string>, etc.
                }else
                {
                    value = j_.at(name).get<std::vector<T>>();
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
                        Serializer sub(mode,format);
                        elem.serialize(sub);
                        arr.push_back(sub.json());
                    }
                    j_[name] = arr;
                }else
                {
                    value.clear();
                    for (auto &elem_json : j_.at(name))
                    {
                        T elem{};
                        Serializer sub(mode,format, elem_json);
                        elem.Serialize(sub);
                        value.push_back(std::move(elem));
                    }
                }
            }
        }

        nlohmann::json json() const { return j_; }

    private:
        nlohmann::json j_;
        std::vector<uint8_t> b_;
    };
}
