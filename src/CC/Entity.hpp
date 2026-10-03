#pragma once
#include "../PCH.hpp"
#include "../json.hpp"
#include "EntityInfo.hpp"
#include "Utils.hpp"
#include "Tile.hpp"
#include "Serializer.hpp"
namespace cc
{
    /**
     * @brief Base class for everything in the world that moves or exists independently of tiles
     *        (people, items, ...). Subclasses override the virtual functions to change how an
     *        entity looks, behaves and is saved.
     *
     * Entities are owned by their planet, and each one is also registered (not owned) in the entity
     * list of the chunk it is standing in.
     */
    class Entity
    {
    public:
        /**
         * @brief Which kind of entity this is. Used to create the right subclass when loading
         *        or receiving an entity (see CreateEntityFromType()).
         * @warning These values are written to save files and sent over the network, so do not
         *          reorder them; add new types at the end.
         */
        enum EntityType : uint16_t
        {
            /// A plain Entity with no extra behaviour.
            NONE = 0,
            /// A Human.
            HUMAN,
            /// An Item.
            ITEM
        };

        /// @brief Creates a plain entity at the origin, with a size of one tile and type NONE.
        Entity() = default;

        /// @brief Virtual so that entities can be deleted through an Entity pointer (as the planet does)
        ///        and the subclass is destroyed properly.
        virtual ~Entity() = default;

        /// What kind of entity this is. Subclasses set it in their constructors.
        EntityType type = NONE;

        /// Position in chunk coordinates of the chunk this entity belongs to. Kept up to date by the
        /// planet as the entity moves (see UpdateChunkPos()).
        sf::Vector2i chunkPos;

        /// Position in tile units (not pixels). Multiply by TILE_SIZE to get the position in world pixels.
        sf::Vector2f position = {0.f, 0.f};

        /// Size in tile units. Scales the drawn rectangle.
        sf::Vector2f size = {1.f, 1.f};

        /// True once the entity has been added to the entity list of the chunk at `chunkPos`.
        /// False while that chunk is not loaded (for example, on a client that has not received it yet).
        bool isInChunk = false;

        /**
         * @brief Builds the vertices to draw this entity: a rectangle of two triangles.
         * @return Six vertices with positions in world pixels and texture coordinates from GetTexCoords().
         *         The rectangle's top-left is at `position` and its size is `size` in tiles.
         */
        virtual std::vector<sf::Vertex> GetVerts()
        {
            sf::Vector2f offsets[6] = {
                {0, 0},
                {TILE_SIZE * size.x, 0},
                {TILE_SIZE * size.x, TILE_SIZE * size.y},
                {0, 0},
                {TILE_SIZE * size.x, TILE_SIZE * size.y},
                {0, TILE_SIZE * size.y}};

            sf::Vector2f texCoords = GetTexCoords();
            std::vector<sf::Vertex> verts;

            for (int i = 0; i < 6; i++)
            {
                sf::Vertex v;

                v.position = position * (float)TILE_SIZE + offsets[i];
                v.texCoords = texCoords + offsets[i];

                verts.push_back(v);
            }

            return verts;
        }

        /**
         * @brief Gets where this entity's texture starts in the entity atlas.
         * @return The top-left pixel of the texture in the atlas, read from the "Entity" entry of
         *         EntityInfo::texturesJson. Subclasses override this to use their own texture.
         */
        virtual sf::Vector2f GetTexCoords()
        {
            return JsonAsVector(EntityInfo::texturesJson["Entity"]);
        }

        /// @brief Advances the entity by one simulation step. Called by the server at its tick rate.
        ///        Does nothing by default; subclasses override it to add behaviour.
        virtual void Tick() {};

        /**
         * @brief Writes or reads the entity's saved state: type, position and size.
         *
         * Subclasses override this, call the base version, and add their own fields. When reading,
         * `type` is overwritten with the stored value, so the right subclass should already have been
         * created with CreateEntityFromType().
         * @param s The serializer to use (its mode decides whether this reads or writes).
         */
        virtual void Serialize(Serializer &s)
        {
            uint16_t t = (uint16_t)type;
            s.field("type", t);
            type = (EntityType)t;
            s.field("position", position);
            s.field("size", size);
        }

        /// @brief Recalculates `chunkPos` from the current `position`. This only updates the number;
        ///        it does not move the entity between chunks' entity lists.
        void UpdateChunkPos()
        {
            chunkPos = TileToChunkPos(position);
        }
    };

    /**
     * @brief Creates a new entity of the subclass matching a type.
     * @param type The kind of entity to create.
     * @return A new entity allocated with `new`, which the caller must own and eventually delete
     *         (usually by giving it to a planet), or nullptr if the type is not recognised.
     */
    Entity *CreateEntityFromType(Entity::EntityType type);
}