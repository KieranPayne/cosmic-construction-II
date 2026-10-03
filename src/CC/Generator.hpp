#pragma once
#include "../PCH.hpp"
#include "Chunk.hpp"
#include "PerlinNoise.hpp"
#include "../json.hpp"
#include "Serializer.hpp"
namespace cc
{
	/**
	 * @brief The steps terrain generation goes through for a chunk, in order.
	 *
	 * A PartialChunk's `nextStage` is the next step still to be done, so the steps before it are complete.
	 */
	enum class GenerationStage : uint16_t
	{
		/// Creates the chunk and uses Perlin noise to choose each background tile's terrain type.
		/// Also stores, in each tile's red channel, how far through its terrain type's noise range it is
		/// (in 5 steps), for the next stage to use.
		SET_TYPES = 0,

		/// Turns each tile's terrain type and blend amount into its final colour, with some
		/// random-looking brightness variation for sand, grass and grassy stone.
		SET_TILE_COLORS,

		/// Not a step: reaching this means generation is complete.
		FINISHED
	};

	class Generator;

	/**
	 * @brief A chunk that is being generated one stage at a time.
	 *
	 * Generator::GenerateChunk() currently runs every stage at once, so partial chunks normally only
	 * exist for the duration of that call, apart from any restored by Generator::Load().
	 */
	class PartialChunk
	{
	public:
		/// The generator that owns this partial chunk. Not owned by this object; supplies the seed.
		Generator *generator;

		/// Position of this chunk in chunk coordinates (not tiles or pixels).
		sf::Vector2i position;

		/// The chunk being built. Null until the first stage (SET_TYPES) has run.
		std::unique_ptr<Chunk> chunk = std::unique_ptr<Chunk>(nullptr);

		/// The next stage to run. Starts at SET_TYPES; the stages before it are done.
		GenerationStage nextStage;

		/**
		 * @brief Creates a partial chunk that has not started generating.
		 * @param generator The generator it belongs to.
		 * @param position The chunk's position in chunk coordinates.
		 */
		PartialChunk(Generator *generator, sf::Vector2i position);

		/// @brief Runs the stage in `nextStage`, then advances `nextStage` to the one after it.
		void GenerateNextStage();

		/**
		 * @brief Runs stages until every stage before `stage` is done.
		 * @param stage The value `nextStage` should reach. Passing FINISHED completes the whole chunk.
		 *              Does nothing if `nextStage` is already at or past it.
		 */
		void GenerateToStage(GenerationStage stage);

		/**
		 * @brief Saves this partial chunk: its chunk data and its generation progress.
		 * @param path The folder to save into, ending in a slash (for example ".../partial chunks/").
		 *             Files are named after the chunk's position.
		 * @warning The chunk must have been created (at least the first stage must have run).
		 */
		void Save(std::string path);

		/**
		 * @brief Loads a partial chunk saved by Save(). The position must already be set.
		 * @param path The same folder that was passed to Save(), ending in a slash.
		 */
		void Load(std::string path);

		/**
		 * @brief Writes or reads the generation progress: the chunk's position and `nextStage`.
		 * @param s The serializer to use (its mode decides whether this reads or writes).
		 * @note The chunk's tiles are not included; they are saved separately by Save().
		 */
		void Serialize(Serializer &s);

		/**
		 * @brief Whether generation has reached a given stage.
		 * @param stage The stage to compare against.
		 * @return true if `nextStage` is `stage` or later.
		 */
		bool NextStageGreaterOrEqual(GenerationStage stage);
	};

	class Planet;

	/**
	 * @brief Procedurally generates a planet's terrain, chunk by chunk, from a seed.
	 *
	 * The same seed always produces the same terrain.
	 */
	class Generator
	{
	public:
		/// Chunks that have started generating but have not been handed out yet, keyed by chunk position.
		/// Finished chunks are removed by GenerateChunk().
		std::unordered_map<sf::Vector2i, std::unique_ptr<PartialChunk>, ChunkHash> partialChunks = {};

		/// The planet this generator belongs to. Set by the planet. Not used by the generator's current code.
		Planet *planet;

		/// Seed for the noise that shapes the terrain. Has no initial value, so set it with SetSeed()
		/// or Load() before generating.
		uint64_t seed;

		/// @brief Creates a generator. Set the seed with SetSeed() before generating chunks.
		Generator() = default;

		/// @brief Sets the seed used for all chunks generated from now on.
		void SetSeed(uint64_t seed);

		/**
		 * @brief Writes or reads the generator's saved state (the seed).
		 * @param s The serializer to use (its mode decides whether this reads or writes).
		 */
		void Serialize(Serializer &s);

		/**
		 * @brief Saves the seed and every partial chunk.
		 * @param path The planet's save folder. Writes "generator" into it and the partial chunks into its
		 *             "partial chunks" subfolder, creating that if needed.
		 */
		void Save(std::string path);

		/**
		 * @brief Loads the seed and the partial chunks saved by Save().
		 * @param path The planet's save folder, as passed to Save().
		 */
		void Load(std::string path);

		/**
		 * @brief Registers a partial chunk with this generator.
		 * @param p The partial chunk. The generator takes ownership, replacing any existing one at the same position.
		 */
		void AddPartialChunk(PartialChunk *p);

		/**
		 * @brief Generates a complete chunk.
		 *
		 * If a partial chunk already exists at the position, generation continues from where it stopped.
		 * @param position The chunk's position in chunk coordinates.
		 * @return The finished chunk. The caller owns it. The generator forgets the partial chunk.
		 */
		Chunk *GenerateChunk(sf::Vector2i position);
	};

}