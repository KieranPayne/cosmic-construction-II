#pragma once
#include "../json.hpp"
#include "../PCH.hpp"
#include "Atlas.hpp"
namespace cc
{
    /**
     * @brief Shared data about entity textures: loads them from disk and records where each one
     *        ends up in the entity texture atlas.
     *
     * Call Init() and then Build() once at startup, after the window exists.
     */
    namespace EntityInfo
    {
        /// The atlas holding every entity texture. Entities are drawn with `atlas.texture`.
        /// Only valid after Build().
        extern Atlas atlas;

        /// The contents of "content/resources/EntityTextures.json". After Init(), every texture file
        /// name in it has been replaced by that texture's [x, y] position in `atlas`, so for example
        /// texturesJson["Human"] is the atlas position of the human texture.
        extern nlohmann::json texturesJson;

        /**
         * @brief Loads "content/resources/EntityTextures.json" and every texture it lists, adding the
         *        textures to `atlas` and rewriting `texturesJson` with their positions.
         * @note Does not build the atlas; call Build() afterwards. Throws if the JSON file is missing or invalid.
         */
        void Init();

        /**
         * @brief Replaces every texture file name inside a JSON value with its position in the atlas.
         *
         * Walks the value recursively. Each string is treated as a path relative to "content/resources/";
         * that texture is loaded and added to `atlas`, and the string is replaced with the array [x, y].
         * If a texture fails to load, a message is printed and the string is left unchanged.
         * @param j The JSON value to rewrite in place.
         */
        void TransformJson(nlohmann::json &j);

        /// @brief Builds `atlas` from all the textures added so far. Call once, after Init().
        void Build();
    }
}