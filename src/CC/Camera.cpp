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
		targetZoom = zoom;
		prevMousePos = {0.f, 0.f};
		// the zoom rate is the (numScrollsToDouble)th root of 2, so that it takes that many scrolls to double the zoom
		int numScrollsToDouble = 3;
		zoomRate = std::pow(2.f, 1.f / numScrollsToDouble);
		zoomSpeed = 25.f;
		moveSpeed = 32.f;
	}

	Camera::Camera()
	{
		position = sf::Vector2f(0.f, 0.f);
		zoom = 1;
		targetZoom = zoom;
		prevMousePos = {0.f, 0.f};
		// the zoom rate is the (numScrollsToDouble)th root of 2, so that it takes that many scrolls to double the zoom
		int numScrollsToDouble = 3;
		zoomRate = std::pow(2.f, 1.f / numScrollsToDouble);
		zoomSpeed = 25.f;
		moveSpeed = 512.f;
	}

	void Camera::Update(float dt, InputState &input)
	{
		// dragging with the middle mouse button pans the camera
		if (input.Pressed(sf::Mouse::Button::Middle))
		{
			mouseStartPos = input.mousePosition;
			cameraStartPos = position;
		}
		if (input.Down(sf::Mouse::Button::Middle))
		{
			sf::Vector2f offset = input.mousePosition - mouseStartPos;
			offset *= zoom;
			position = cameraStartPos - offset;
		}

		// scrolling changes the target zoom (scroll up zooms in)
		if (input.scroll.y != 0 && !input.Down(sf::Keyboard::Key::LControl))
		{
			float scrollAmount = std::abs(input.scroll.y);
			if (input.scroll.y > 0)
			{
				targetZoom /= ((zoomRate - 1) * scrollAmount) + 1;
			}
			else
			{
				targetZoom *= ((zoomRate - 1) * scrollAmount) + 1;
			}
			// minimum and maximum zoom
			if (targetZoom < 0.01f)
			{
				targetZoom *= zoomRate;
			}
			if (targetZoom > 8.f)
			{
				targetZoom /= zoomRate;
			}
		}

		// ease the zoom towards the target using exponential decay, which is frame rate independent.
		// from "Lerp smoothing is broken" https://www.youtube.com/watch?v=LSNQuFEDOyQ&t=3050s&ab_channel=FreyaHolm%C3%A9r
		zoom = targetZoom + (zoom - targetZoom) * std::exp(-zoomSpeed * dt);
		prevMousePos = input.mousePosition;
	}

	sf::FloatRect Camera::toFloatRect(sf::RenderTarget *target)
	{
		float zoomStepSize = 0.0001f;
		float effectiveZoom = std::round(zoom / zoomStepSize) * zoomStepSize;
		sf::Vector2f effectivePos{std::round(position.x),std::round(position.y)};
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
		s.field("targetZoom", targetZoom);
	}

	sf::Vector2f Camera::ToWorldPos(sf::Vector2f screenPosition, sf::RenderTarget *target)
	{
		return (screenPosition - (sf::Vector2f)target->getSize() / 2.f) * zoom + position;
	}
}