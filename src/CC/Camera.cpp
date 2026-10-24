#include "Camera.hpp"
#include "../Input/Input.hpp"
#include "../Main.hpp"
#include "../json.hpp"
#include <cmath>
namespace cc
{
	Camera::Camera(sf::Vector2f position, float zoom)
	{
		this->position = position;
		this->zoom = zoom;
		prevMousePos = {0.f, 0.f};
		// the zoom rate is the (numScrollsToDouble)th root of 2, so that it takes that many scrolls to double the zoom
		int numScrollsToDouble = 3;
		zoomRate = std::pow(2.f, 1.f / numScrollsToDouble);
		moveSpeed = 32.f;
	}

	Camera::Camera()
	{
		position = sf::Vector2f(0.f, 0.f);
		zoom = 1;
		prevMousePos = {0.f, 0.f};
		// the zoom rate is the (numScrollsToDouble)th root of 2, so that it takes that many scrolls to double the zoom
		int numScrollsToDouble = 3;
		zoomRate = std::pow(2.f, 1.f / numScrollsToDouble);
		moveSpeed = 512.f;
	}

	void Camera::Update(float dt, InputState &input, sf::RenderTarget* target)
	{
		sf::Vector2f screenCenter = (sf::Vector2f)target->getSize() / 2.f;
		panning = false;
		// dragging with the middle mouse button pans the camera
		if (input.Pressed(sf::Mouse::Button::Middle) || (input.Down(sf::Keyboard::Key::Space) && input.Pressed(sf::Mouse::Button::Left)))
		{
			mouseStartPos = input.mousePosition;
			cameraStartPos = position;
		}
		if (input.Down(sf::Mouse::Button::Middle) || (input.Down(sf::Keyboard::Key::Space) && input.Down(sf::Mouse::Button::Left)))
		{
			panning = true;
			sf::Vector2f offset = input.mousePosition - mouseStartPos;
			offset *= zoom;
			position = cameraStartPos - offset;
		}

		// scrolling zooms instantly towards/away from the mouse cursor (scroll up zooms in)
		if (input.scroll.y != 0 && !input.Down(sf::Keyboard::Key::LControl))
		{
			float scrollAmount = std::abs(input.scroll.y);
			float oldZoom = zoom;
			float newZoom = zoom;

			if (input.scroll.y > 0)
			{
				newZoom /= ((zoomRate - 1) * scrollAmount) + 1;
			}
			else
			{
				newZoom *= ((zoomRate - 1) * scrollAmount) + 1;
			}

			// minimum and maximum zoom
			newZoom = std::clamp(newZoom, 0.01f, 8.f);

			if (newZoom != oldZoom)
			{
				// mouse offset from the screen centre, in screen pixels.
				// `screenCenter` must be the size of the render target / 2
				sf::Vector2f mouseFromCenter = input.mousePosition - screenCenter;

				// world point under the mouse before zooming:
				//   worldPoint = mouseFromCenter * oldZoom + position
				// we want the same world point under the mouse after zooming:
				//   worldPoint = mouseFromCenter * newZoom + newPosition
				// so: newPosition = position + mouseFromCenter * (oldZoom - newZoom)
				position += mouseFromCenter * (oldZoom - newZoom);

				zoom = newZoom;
			}
		}

		prevMousePos = input.mousePosition;
	}

	sf::FloatRect Camera::toFloatRect(sf::RenderTarget *target)
	{
		float zoomStepSize = 0.0001f;
		float effectiveZoom = std::round(zoom / zoomStepSize) * zoomStepSize;
		sf::Vector2f effectivePos{std::round(position.x), std::round(position.y)};
		float width = static_cast<float>(target->getSize().x);
		float height = static_cast<float>(target->getSize().y);

		float viewWidth = width * effectiveZoom;
		float viewHeight = height * effectiveZoom;

		sf::FloatRect rect(
			{effectivePos.x - viewWidth / 2.f,
			 effectivePos.y - viewHeight / 2.f},
			{viewWidth, viewHeight});

		// Snap the camera to the world-space size of one screen pixel.
		float pixelSize = effectiveZoom;

		rect.position.x = std::round(rect.position.x / pixelSize) * pixelSize;
		rect.position.y = std::round(rect.position.y / pixelSize) * pixelSize;

		return rect;
	}

	void Camera::SetView(sf::RenderTarget *target)
	{
		sf::FloatRect rect = toFloatRect(target);
		auto view = sf::View(rect);
		target->setView(view);
	}

	void Camera::Serialize(Serializer &s)
	{
		s.field("position", position);
		s.field("zoom", zoom);
	}

	sf::Vector2f Camera::ToWorldPos(sf::Vector2f screenPosition, sf::RenderTarget *target)
	{
		return (screenPosition - (sf::Vector2f)target->getSize() / 2.f) * zoom + position;
	}
	bool Camera::GetPanning()
	{
		return panning;
	}
}