#pragma once
#include "../Input/Input.hpp"
#include "../PCH.hpp"
#include "../json.hpp"
#include "Serializer.hpp"
namespace cc
{
	/**
	 * @brief A 2D camera with drag-to-pan and smooth scroll-to-zoom.
	 *
	 * All positions are in world pixels. `zoom` is how many world pixels one screen pixel covers,
	 * so a larger zoom shows more of the world (zoomed out) and a smaller zoom shows less (zoomed in).
	 */
	class Camera
	{
	public:
		/// World position at the centre of the view.
		sf::Vector2f position;

		/// Screen position of the mouse when the current left-button drag began.
		sf::Vector2f mouseStartPos;

		/// Value of `position` when the current left-button drag began.
		sf::Vector2f cameraStartPos;

		/// Screen position of the mouse at the end of the previous Update(). Stored but not read by Camera itself.
		sf::Vector2f prevMousePos;

		/// The zoom currently in effect. Eases towards `targetZoom` every Update().
		float zoom;

		/// The zoom the camera is moving towards. Changed by scrolling; clamped to roughly 0.01 to 8.
		float targetZoom;

		/// Factor `targetZoom` changes by per scroll step. Set so that three steps double (or halve) the zoom.
		float zoomRate;

		/// How quickly `zoom` catches up with `targetZoom`, per second. Higher is snappier.
		float zoomSpeed;

		/// Camera movement speed. Not used by Camera itself; kept for code that moves the camera.
		float moveSpeed;

		~Camera() = default;

		/// @brief Creates a camera at the world origin with a zoom of 1.
		Camera();

		/**
		 * @brief Creates a camera at a given position and zoom.
		 * @param position World position for the centre of the view.
		 * @param zoom Initial zoom. Also used as the target zoom, so there is no initial zoom animation.
		 */
		Camera(sf::Vector2f position, float zoom);

		/**
		 * @brief Converts a position on screen to a position in the world.
		 * @param screenPosition Position in pixels relative to the render target's top-left corner (for example, the mouse position).
		 * @param target The render target the camera is drawing to, used for its size.
		 * @return The world position under that screen position, using the current `position` and `zoom`.
		 * @note Does not apply the pixel snapping that toFloatRect() does, so it can differ by less than one screen pixel.
		 */
		sf::Vector2f ToWorldPos(sf::Vector2f screenPosition, sf::RenderTarget *target);

		/**
		 * @brief Processes input and animates the zoom. Call once per frame.
		 *
		 * Dragging with the left mouse button pans the camera. Scrolling changes `targetZoom`
		 * (ignored while left control is held), and `zoom` then eases towards it.
		 * @param dt Seconds since the previous frame.
		 * @param input This frame's input state.
		 */
		void Update(float dt, InputState &input);

		/**
		 * @brief Returns the area of the world the camera can currently see.
		 *
		 * The top-left corner is snapped to a multiple of `zoom` (one screen pixel in world units)
		 * to avoid shimmering when the camera moves.
		 * @param target The render target being drawn to, used for its size in pixels.
		 * @return The visible world rectangle, in world pixels.
		 */
		sf::FloatRect toFloatRect(sf::RenderTarget *target);

		/**
		 * @brief Applies this camera's view to a render target, so that subsequent drawing is in world coordinates.
		 * @param target The render target to set the view on. Its previous view is not restored.
		 */
		void SetView(sf::RenderTarget *target);

		/**
		 * @brief Writes or reads the camera's saved state.
		 *
		 * Only `position` and `targetZoom` are included; `zoom` is not, so after reading, `zoom`
		 * still has its old value and eases to the loaded `targetZoom`.
		 * @param s The serializer to use (its mode decides whether this reads or writes).
		 */
		void Serialize(Serializer &s);
	};
}