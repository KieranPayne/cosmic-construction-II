#pragma once
#include "../PCH.hpp"
namespace cc
{
	/**
	 * @brief Packs many small textures into a single large texture (a "texture atlas").
	 *
	 * Usage: add every texture with AddTexture(), then call Build() once to produce `texture`.
	 * (The constructor that takes a vector does both steps for you.)
	 *
	 * Details worth knowing:
	 *  - The atlas always starts with a 1x1 white pixel at (0, 0). Vertices that have no texture
	 *    coordinates sample it, so they are drawn with just their vertex colour. It is stored as
	 *    entry 0 of `textures` / `positions` while the atlas is being filled, and Build() removes it,
	 *    so afterwards index i is the i-th texture you added.
	 *  - Every texture is surrounded by a 1 pixel border copied from its own edge pixels. This stops
	 *    rounding errors when sampling at a texture's edge from picking up a neighbouring texture
	 *    (visible as seams between tiles).
	 *  - Textures are placed left to right in rows; a new row starts when the next texture would
	 *    not fit within `maxSize`.
	 */
	class Atlas
	{
	public:
		/// The finished atlas texture. Only valid after Build() has been called.
		sf::Texture texture;

		/// Height in pixels of the row currently being filled: the tallest texture in it so far, border included.
		int rowHeight = 0;

		/// Width in pixels already used in the row currently being filled, borders included.
		int rowWidth = 0;

		/// Width in pixels of the whole atlas (the widest row so far).
		int totalWidth = 0;

		/// Height in pixels of all completed rows. Build() adds the final row, so after Build()
		/// this is the full height of the atlas.
		int totalHeight = 0;

		/// Largest width or height a texture may have on this GPU. Queried when the Atlas is
		/// constructed, so an OpenGL context must exist by then.
		int maxSize = sf::Texture::getMaximumSize();

		/// Copies of the source textures in the order they were added. Parallel to `positions`.
		std::vector<sf::Texture> textures;

		/// Top-left pixel of each texture's own pixels inside the atlas (not of its border), in the
		/// same order as `textures`. Use these as texture coordinates.
		std::vector<sf::Vector2i> positions;

		/**
		 * @brief Creates an atlas from a list of textures and builds it immediately.
		 * @param initialTextures Textures to pack, in the order their positions will be reported.
		 */
		Atlas(std::vector<sf::Texture> &initialTextures);

		/**
		 * @brief Creates an empty atlas containing only the white pixel.
		 * @note Add textures with AddTexture() and then call Build().
		 */
		Atlas();

		/**
		 * @brief Reserves space for a texture and records where it will go.
		 *
		 * Nothing is drawn into the atlas until Build() is called.
		 * @param texture The texture to add. It is copied, so the original can be discarded.
		 * @note Appends to `textures` and `positions`.
		 */
		void AddTexture(sf::Texture texture);

		/**
		 * @brief Draws every added texture (and its border) into one image and uploads it to `texture`.
		 *
		 * Also removes the internal white pixel entry from `textures` and `positions`, and writes
		 * the result to "atlas.png" in the save data directory.
		 * @warning Call exactly once, after all textures have been added. Calling it again would
		 *          corrupt the layout, because it adds the last row's height and removes entry 0 each time.
		 */
		void Build();
	};
}