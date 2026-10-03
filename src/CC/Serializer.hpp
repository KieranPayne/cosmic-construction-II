#pragma once
#include "../json.hpp"
#include "../PCH.hpp"
namespace cc
{
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
     *
     * Typical use:
     * @code
     * Serializer w(Serializer::Mode::WRITE, Serializer::Format::JSON);
     * player.Serialize(w);
     * nlohmann::json j = w.json();
     *
     * Serializer r(Serializer::Mode::READ, Serializer::Format::JSON, j);
     * player.Serialize(r);
     * @endcode
     */
    class Serializer
    {
    public:
        /// Whether a serializer fills values in or stores them.
        enum class Mode
        {
            /// field() fills in each value from the stored data.
            READ,
            /// field() stores each value.
            WRITE
        };

        /// Whether this serializer reads or writes.
        Mode mode;

        /// The kind of data a serializer works with.
        enum class Format
        {
            /// Human-readable json, with fields stored by name.
            JSON,
            /// Compact bytes, with fields stored in order and no names.
            BINARY
        };

        /// Whether this serializer works with json or binary.
        Format format;

        /**
         * @brief Creates a serializer.
         * @param mode READ to load values, WRITE to store them.
         * @param format JSON or BINARY.
         * @param j The json to read from. Only used when reading JSON.
         * @param b The bytes to read from. Only used when reading BINARY.
         */
        Serializer(Mode mode, Format format, nlohmann::json j = {}, std::vector<uint8_t> b = {})
        {
            this->mode = mode;
            this->format = format;
            j_ = j;
            b_ = b;
        }

        /**
         * @brief Writes or reads a number, bool or string.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The value to store, or to fill in when reading.
         */
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

        /**
         * @brief Writes or reads an sf::Vector2. In JSON it is stored as {"x": ..., "y": ...}.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The vector to store, or to fill in when reading.
         * @note SFML's vectors have no Serialize() function of their own, so they get their own overloads.
         */
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

        /**
         * @brief Writes or reads an sf::Vector3. In JSON it is stored as {"x": ..., "y": ..., "z": ...}.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The vector to store, or to fill in when reading.
         */
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

        /**
         * @brief Writes or reads an object that has its own `Serialize(Serializer&)` function.
         *
         * In JSON the object becomes a nested json object under `name`. In BINARY its fields follow on directly.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The object to store, or to fill in when reading.
         */
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

        /**
         * @brief Writes or reads an object through a pointer. The object needs its own `Serialize(Serializer&)` function.
         *
         * Stored the same way as the by-reference overload.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The object to store, or to fill in when reading.
         * @warning The pointer is passed by value. When reading, a null pointer gets a new object, but the caller
         *          never sees it (and it is leaked), so always pass an existing object when reading.
         */
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

        /**
         * @brief Writes or reads a vector of numbers, bools or strings.
         *
         * In JSON it is a json array. In BINARY it is the element count followed by each element.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The vector to store, or to replace when reading.
         */
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
                        field(name, elem); // reuses the scalar overload for each element
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

        /**
         * @brief Writes or reads a vector of objects that each have their own `Serialize(Serializer&)` function.
         *
         * In JSON it is an array with one nested object per element. In BINARY it is the element count followed by
         * each element. When reading, the vector is cleared and refilled, and the element type must be default constructible.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The vector to store, or to replace when reading.
         */
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

        /**
         * @brief Writes or reads a std::size_t.
         * @param name The field's name (used in JSON; ignored in BINARY).
         * @param value The value to store, or to fill in when reading.
         */
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

        /// @brief Gets a copy of the json written so far (JSON format). Call after writing to get the result.
        nlohmann::json json() const { return j_; }

        /// @brief Gets a copy of the bytes written so far (BINARY format). Call after writing to get the result.
        std::vector<uint8_t> binary() const { return b_; }

    private:
        /// The json being written to or read from (JSON format).
        nlohmann::json j_;

        /// The bytes being written to or read from (BINARY format).
        std::vector<uint8_t> b_;

        /// Where the next binary read starts in `b_`.
        size_t readPos_ = 0;

        /// @brief Appends a value's bytes to `b_`. Only for types that can be safely copied as raw bytes.
        template <typename T>
        void writeRaw(const T &value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "writeRaw requires a trivially copyable type");
            const uint8_t *p = reinterpret_cast<const uint8_t *>(&value);
            b_.insert(b_.end(), p, p + sizeof(T));
        }

        /// @brief Fills a value from the bytes at `readPos_` in `b_`, then moves `readPos_` past them. Does not check that enough bytes remain.
        template <typename T>
        void readRaw(T &value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "readRaw requires a trivially copyable type");
            std::memcpy(&value, b_.data() + readPos_, sizeof(T));
            readPos_ += sizeof(T);
        }
    };
}