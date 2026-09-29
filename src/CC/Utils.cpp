#include "Utils.hpp"
#include "Chunk.hpp"
#include "Serializer.hpp"
#include "Entity.hpp"
namespace cc
{
	std::vector<std::string> Split(std::string str, char splitChar)
	{
		std::string curr = "";
		std::vector<std::string> result;
		for (int i = 0; i < str.size(); i++)
		{
			if (str[i] == splitChar)
			{

				result.push_back(curr);
				curr = "";
				continue;
			}
			curr += str[i];
		}
		result.push_back(curr);
		return result;
	}

	sf::Color HexToColor(const std::string &hex)
	{
		std::string value = hex;

		// Remove '#' if present
		if (!value.empty() && value[0] == '#')
		{
			value.erase(0, 1);
		}

		// Must be either 6 (RGB) or 8 (RGBA) characters
		if (value.length() != 6 && value.length() != 8)
		{
			throw std::invalid_argument("Invalid hex color string");
		}

		// Parse each component
		unsigned int r = std::stoul(value.substr(0, 2), nullptr, 16);
		unsigned int g = std::stoul(value.substr(2, 2), nullptr, 16);
		unsigned int b = std::stoul(value.substr(4, 2), nullptr, 16);
		unsigned int a = 255; // Default alpha

		if (value.length() == 8)
		{
			a = std::stoul(value.substr(6, 2), nullptr, 16);
		}

		return sf::Color(r, g, b, a);
	}
	sf::Vector2f JsonAsVector(nlohmann::json &j)
	{
		return sf::Vector2f(
			(float)j[0].get<int>(),
			(float)j[1].get<int>());
	}
	int fastFloorDiv(int v)
	{
		return (v >= 0) ? (v >> 5) : -((-v - 1) >> 5) - 1;
	}
	int TileToChunkPos(int pos)
	{
		return fastFloorDiv(pos);
	}
	sf::Vector2i TileToChunkPos(sf::Vector2i &pos)
	{
		return {
			fastFloorDiv(pos.x),
			fastFloorDiv(pos.y),
		};
	}
	sf::Vector2i TileToChunkPos(sf::Vector2f &pos)
	{
		return {
			static_cast<int>(std::floor(pos.x / 32.0f)),
			static_cast<int>(std::floor(pos.y / 32.0f))};
	}
	ImGuiKey keycodeToImGuiKey(sf::Keyboard::Key code)
	{
		switch (code)
		{
		case sf::Keyboard::Key::Tab:
			return ImGuiKey_Tab;
		case sf::Keyboard::Key::Left:
			return ImGuiKey_LeftArrow;
		case sf::Keyboard::Key::Right:
			return ImGuiKey_RightArrow;
		case sf::Keyboard::Key::Up:
			return ImGuiKey_UpArrow;
		case sf::Keyboard::Key::Down:
			return ImGuiKey_DownArrow;
		case sf::Keyboard::Key::PageUp:
			return ImGuiKey_PageUp;
		case sf::Keyboard::Key::PageDown:
			return ImGuiKey_PageDown;
		case sf::Keyboard::Key::Home:
			return ImGuiKey_Home;
		case sf::Keyboard::Key::End:
			return ImGuiKey_End;
		case sf::Keyboard::Key::Insert:
			return ImGuiKey_Insert;
		case sf::Keyboard::Key::Delete:
			return ImGuiKey_Delete;
		case sf::Keyboard::Key::Backspace:
			return ImGuiKey_Backspace;
		case sf::Keyboard::Key::Space:
			return ImGuiKey_Space;
		case sf::Keyboard::Key::Enter:
			return ImGuiKey_Enter;
		case sf::Keyboard::Key::Escape:
			return ImGuiKey_Escape;
		case sf::Keyboard::Key::Apostrophe:
			return ImGuiKey_Apostrophe;
		case sf::Keyboard::Key::Comma:
			return ImGuiKey_Comma;
		case sf::Keyboard::Key::Hyphen:
			return ImGuiKey_Minus;
		case sf::Keyboard::Key::Period:
			return ImGuiKey_Period;
		case sf::Keyboard::Key::Slash:
			return ImGuiKey_Slash;
		case sf::Keyboard::Key::Semicolon:
			return ImGuiKey_Semicolon;
		case sf::Keyboard::Key::Equal:
			return ImGuiKey_Equal;
		case sf::Keyboard::Key::LBracket:
			return ImGuiKey_LeftBracket;
		case sf::Keyboard::Key::Backslash:
			return ImGuiKey_Backslash;
		case sf::Keyboard::Key::RBracket:
			return ImGuiKey_RightBracket;
		case sf::Keyboard::Key::Grave:
			return ImGuiKey_GraveAccent;
		// case : return ImGuiKey_CapsLock;
		// case : return ImGuiKey_ScrollLock;
		// case : return ImGuiKey_NumLock;
		// case : return ImGuiKey_PrintScreen;
		case sf::Keyboard::Key::Pause:
			return ImGuiKey_Pause;
		case sf::Keyboard::Key::Numpad0:
			return ImGuiKey_Keypad0;
		case sf::Keyboard::Key::Numpad1:
			return ImGuiKey_Keypad1;
		case sf::Keyboard::Key::Numpad2:
			return ImGuiKey_Keypad2;
		case sf::Keyboard::Key::Numpad3:
			return ImGuiKey_Keypad3;
		case sf::Keyboard::Key::Numpad4:
			return ImGuiKey_Keypad4;
		case sf::Keyboard::Key::Numpad5:
			return ImGuiKey_Keypad5;
		case sf::Keyboard::Key::Numpad6:
			return ImGuiKey_Keypad6;
		case sf::Keyboard::Key::Numpad7:
			return ImGuiKey_Keypad7;
		case sf::Keyboard::Key::Numpad8:
			return ImGuiKey_Keypad8;
		case sf::Keyboard::Key::Numpad9:
			return ImGuiKey_Keypad9;
		// case : return ImGuiKey_KeypadDecimal;
		case sf::Keyboard::Key::Divide:
			return ImGuiKey_KeypadDivide;
		case sf::Keyboard::Key::Multiply:
			return ImGuiKey_KeypadMultiply;
		case sf::Keyboard::Key::Subtract:
			return ImGuiKey_KeypadSubtract;
		case sf::Keyboard::Key::Add:
			return ImGuiKey_KeypadAdd;
		// case : return ImGuiKey_KeypadEnter;
		// case : return ImGuiKey_KeypadEqual;
		case sf::Keyboard::Key::LControl:
			return ImGuiKey_LeftCtrl;
		case sf::Keyboard::Key::LShift:
			return ImGuiKey_LeftShift;
		case sf::Keyboard::Key::LAlt:
			return ImGuiKey_LeftAlt;
		case sf::Keyboard::Key::LSystem:
			return ImGuiKey_LeftSuper;
		case sf::Keyboard::Key::RControl:
			return ImGuiKey_RightCtrl;
		case sf::Keyboard::Key::RShift:
			return ImGuiKey_RightShift;
		case sf::Keyboard::Key::RAlt:
			return ImGuiKey_RightAlt;
		case sf::Keyboard::Key::RSystem:
			return ImGuiKey_RightSuper;
		case sf::Keyboard::Key::Menu:
			return ImGuiKey_Menu;
		case sf::Keyboard::Key::Num0:
			return ImGuiKey_0;
		case sf::Keyboard::Key::Num1:
			return ImGuiKey_1;
		case sf::Keyboard::Key::Num2:
			return ImGuiKey_2;
		case sf::Keyboard::Key::Num3:
			return ImGuiKey_3;
		case sf::Keyboard::Key::Num4:
			return ImGuiKey_4;
		case sf::Keyboard::Key::Num5:
			return ImGuiKey_5;
		case sf::Keyboard::Key::Num6:
			return ImGuiKey_6;
		case sf::Keyboard::Key::Num7:
			return ImGuiKey_7;
		case sf::Keyboard::Key::Num8:
			return ImGuiKey_8;
		case sf::Keyboard::Key::Num9:
			return ImGuiKey_9;
		case sf::Keyboard::Key::A:
			return ImGuiKey_A;
		case sf::Keyboard::Key::B:
			return ImGuiKey_B;
		case sf::Keyboard::Key::C:
			return ImGuiKey_C;
		case sf::Keyboard::Key::D:
			return ImGuiKey_D;
		case sf::Keyboard::Key::E:
			return ImGuiKey_E;
		case sf::Keyboard::Key::F:
			return ImGuiKey_F;
		case sf::Keyboard::Key::G:
			return ImGuiKey_G;
		case sf::Keyboard::Key::H:
			return ImGuiKey_H;
		case sf::Keyboard::Key::I:
			return ImGuiKey_I;
		case sf::Keyboard::Key::J:
			return ImGuiKey_J;
		case sf::Keyboard::Key::K:
			return ImGuiKey_K;
		case sf::Keyboard::Key::L:
			return ImGuiKey_L;
		case sf::Keyboard::Key::M:
			return ImGuiKey_M;
		case sf::Keyboard::Key::N:
			return ImGuiKey_N;
		case sf::Keyboard::Key::O:
			return ImGuiKey_O;
		case sf::Keyboard::Key::P:
			return ImGuiKey_P;
		case sf::Keyboard::Key::Q:
			return ImGuiKey_Q;
		case sf::Keyboard::Key::R:
			return ImGuiKey_R;
		case sf::Keyboard::Key::S:
			return ImGuiKey_S;
		case sf::Keyboard::Key::T:
			return ImGuiKey_T;
		case sf::Keyboard::Key::U:
			return ImGuiKey_U;
		case sf::Keyboard::Key::V:
			return ImGuiKey_V;
		case sf::Keyboard::Key::W:
			return ImGuiKey_W;
		case sf::Keyboard::Key::X:
			return ImGuiKey_X;
		case sf::Keyboard::Key::Y:
			return ImGuiKey_Y;
		case sf::Keyboard::Key::Z:
			return ImGuiKey_Z;
		case sf::Keyboard::Key::F1:
			return ImGuiKey_F1;
		case sf::Keyboard::Key::F2:
			return ImGuiKey_F2;
		case sf::Keyboard::Key::F3:
			return ImGuiKey_F3;
		case sf::Keyboard::Key::F4:
			return ImGuiKey_F4;
		case sf::Keyboard::Key::F5:
			return ImGuiKey_F5;
		case sf::Keyboard::Key::F6:
			return ImGuiKey_F6;
		case sf::Keyboard::Key::F7:
			return ImGuiKey_F7;
		case sf::Keyboard::Key::F8:
			return ImGuiKey_F8;
		case sf::Keyboard::Key::F9:
			return ImGuiKey_F9;
		case sf::Keyboard::Key::F10:
			return ImGuiKey_F10;
		case sf::Keyboard::Key::F11:
			return ImGuiKey_F11;
		case sf::Keyboard::Key::F12:
			return ImGuiKey_F12;
		default:
			break;
		}
		return ImGuiKey_None;
	}
	std::vector<uint8_t> ReadBytesFromPacket(sf::Packet &packet)
	{
		uint64_t n;
		packet >> n;
		std::vector<uint8_t> result;
		result.reserve(n);
		for (int i = 0; i < n; i++)
		{
			uint8_t byte;
			packet >> byte;
			result.push_back(byte);
		}
		return result;
	}
	void AppendEntityToPacket(sf::Packet &packet, Entity *e)
	{
		packet << (uint16_t)e->type;
		Serializer s(Serializer::Mode::WRITE, Serializer::Format::BINARY);
		e->Serialize(s);
		auto data = s.binary();
		packet << data.size();
		packet.append(data.data(), data.size());
	}
	Entity *LoadEntityFromPacket(sf::Packet &packet)
	{
		uint16_t type;
		packet >> type;
		Entity *e = CreateEntityFromType((Entity::EntityType)type);
		std::vector<uint8_t> data = ReadBytesFromPacket(packet);
		Serializer s(Serializer::Mode::READ, Serializer::Format::BINARY, {}, data);
		e->Serialize(s);
		return e;
	}
	sf::Color UsernameToColor(std::string& username)
	{
		// FNV-1a hash (32-bit): stable across platforms and runs
		std::uint32_t hash = 2166136261u;
		for (unsigned char c : username)
		{
			hash ^= c;
			hash *= 16777619u;
		}

		// Map hash to a hue in [0, 360)
		float hue = static_cast<float>(hash % 360);

		// HSV -> RGB with S = 1, V = 1
		float h = hue / 60.f;								// sector 0..5
		float x = 1.f - std::fabs(std::fmod(h, 2.f) - 1.f); // secondary component

		float r = 0.f, g = 0.f, b = 0.f;
		switch (static_cast<int>(h))
		{
		case 0:
			r = 1.f;
			g = x;
			b = 0.f;
			break;
		case 1:
			r = x;
			g = 1.f;
			b = 0.f;
			break;
		case 2:
			r = 0.f;
			g = 1.f;
			b = x;
			break;
		case 3:
			r = 0.f;
			g = x;
			b = 1.f;
			break;
		case 4:
			r = x;
			g = 0.f;
			b = 1.f;
			break;
		default:
			r = 1.f;
			g = 0.f;
			b = x;
			break;
		}

		return sf::Color(
			static_cast<std::uint8_t>(std::round(r * 255.f)),
			static_cast<std::uint8_t>(std::round(g * 255.f)),
			static_cast<std::uint8_t>(std::round(b * 255.f)));
	}
}