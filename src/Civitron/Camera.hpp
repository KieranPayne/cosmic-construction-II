#pragma once
#include "../Input/Input.hpp"
#include "../PCH.hpp"
#include "../json.hpp"
namespace Civitron
{
	class Camera
	{
	public:
		sf::Vector2f position;
		sf::Vector2f mouseStartPos;
		sf::Vector2f cameraStartPos;
		sf::Vector2f prevMousePos;
		float zoom;
		float targetZoom;
		float zoomRate;
		float zoomSpeed;
		float moveSpeed;
		int viewHeight;
		~Camera();
		Camera();
		Camera(sf::Vector2f position, float zoom);
		sf::Vector2f ToWorldPos(sf::Vector2f screenPosition, sf::RenderTarget* target);
		void Update(float dt, InputState &input);
		sf::FloatRect toFloatRect(sf::RenderTarget *target);
		void SetView(sf::RenderTarget *target);
		nlohmann::json ToJson();
		void FromJson(nlohmann::json j);
	};
}
