#include "Camera.hpp"
#include "../Input/Input.hpp"
#include "../Main.hpp"
#include "../json.hpp"
#include <cmath>
namespace Civitron
{
	Camera::Camera(sf::Vector2f position, float zoom)
	{
		this->position = position;
		this->zoom = zoom;
		targetZoom = zoom;
		prevMousePos = {0.f, 0.f};
		// the zoom rate is the (numscrollstodouble)th root of 2, so that it takes that many scrolls to double the zoom
		int numScrollsToDouble = 3;
		zoomRate = pow(2.f, 1.f / numScrollsToDouble);
		zoomSpeed = 25.f;
		moveSpeed = 32.f;
	}
	Camera::Camera()
	{
		position = sf::Vector2f(0.f, 0.f);
		zoom = 1;
		targetZoom = zoom;
		prevMousePos = {0.f, 0.f};
		// the zoom rate is the (numscrollstodouble)th root of 2, so that it takes that many scrolls to double the zoom
		int numScrollsToDouble = 3;
		zoomRate = pow(2.f, 1.f / numScrollsToDouble);
		zoomSpeed = 25.f;
		moveSpeed = 512.f;
	}

	void Camera::Update(float dt, InputState &input)
	{
		if (input.Pressed(sf::Mouse::Button::Left))
		{
			mouseStartPos = input.mousePosition;
			cameraStartPos = position;
		}
		if (input.Down(sf::Mouse::Button::Left))
		{
			sf::Vector2f offset = input.mousePosition - mouseStartPos;
			offset *= zoom;
			position = cameraStartPos - offset;
		}
		if (input.scroll.y != 0 && !input.Down(sf::Keyboard::Key::LControl))
		{
			if (input.scroll.y > 0)
			{
				targetZoom /= (((zoomRate - 1) * abs(input.scroll.y)) + 1);
			}
			else
			{
				targetZoom *= (((zoomRate - 1) * abs(input.scroll.y)) + 1);
			}
			// minimum and maximum zoom
			if (targetZoom < 0.01f)
			{
				targetZoom *= zoomRate;
			}
			if (targetZoom > 4.f)
			{
				targetZoom /= zoomRate;
			}
		}
		// exponential decay algorithm from "Lerp smoothing is broken" https://www.youtube.com/watch?v=LSNQuFEDOyQ&t=3050s&ab_channel=FreyaHolm%C3%A9r
		zoom = targetZoom + (zoom - targetZoom) * exp(-zoomSpeed * dt);
		prevMousePos = input.mousePosition;
	}
	sf::FloatRect Camera::toFloatRect(sf::RenderTarget *target)
	{
		
		// get float rect representing camera
		int width = target->getSize().x;
		int height = target->getSize().y;
		sf::FloatRect rect = sf::FloatRect({position.x - width * zoom / 2.f, position.y - height * zoom / 2.f}, {width * zoom, height * zoom});
		double pixelSize = zoom;
		rect.position.x = std::round(rect.position.x / pixelSize) * pixelSize;
		rect.position.y = std::round(rect.position.y / pixelSize) * pixelSize;
		// rect.size.x = round(rect.size.x / pixelSize) * pixelSize;
		// rect.size.y = round(rect.size.y / pixelSize) * pixelSize;	
		return rect;
	}

	void Camera::SetView(sf::RenderTarget *target)
	{
		sf::FloatRect rect = toFloatRect(target);
		auto view = sf::View(rect);
		target->setView(view);
	}

	nlohmann::json Camera::ToJson()
	{
		nlohmann::json j;
		j["position"] = {position.x, position.y};
		j["targetZoom"] = targetZoom;
		return j;
	}

	void Camera::FromJson(nlohmann::json j)
	{
		targetZoom = j["targetZoom"];
		zoom = j["targetZoom"];
		position = {j["position"][0], j["position"][1]};
	}

	Camera::~Camera()
	{
		// delete hitbox;
	}

	sf::Vector2f Camera::ToWorldPos(sf::Vector2f screenPosition, sf::RenderTarget* target){
		return (screenPosition - (sf::Vector2f)target->getSize() / 2.f) * zoom + position;
	}
}