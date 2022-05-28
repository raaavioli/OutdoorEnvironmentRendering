#pragma once

#include <map>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


enum class Key : int
{
	A = GLFW_KEY_A,
	B = GLFW_KEY_B,
	C = GLFW_KEY_C,
	D = GLFW_KEY_D,
	E = GLFW_KEY_E,
	F = GLFW_KEY_F,
	G = GLFW_KEY_G,
	H = GLFW_KEY_H,
	I = GLFW_KEY_I,
	J = GLFW_KEY_J,
	K = GLFW_KEY_K,
	L = GLFW_KEY_L,
	M = GLFW_KEY_M,
	N = GLFW_KEY_N,
	O = GLFW_KEY_O,
	P = GLFW_KEY_P,
	Q = GLFW_KEY_Q,
	R = GLFW_KEY_R,
	S = GLFW_KEY_S,
	T = GLFW_KEY_T,
	U = GLFW_KEY_U,
	V = GLFW_KEY_V,
	W = GLFW_KEY_W,
	X = GLFW_KEY_X,
	Y = GLFW_KEY_Y,
	Z = GLFW_KEY_Z,
	SPACE = GLFW_KEY_SPACE,
	L_SHIFT = GLFW_KEY_LEFT_SHIFT,
	R_SHIFT = GLFW_KEY_RIGHT_SHIFT,
	L_ALT = GLFW_KEY_LEFT_ALT,
	R_ALT = GLFW_KEY_RIGHT_ALT,
	L_CTRL = GLFW_KEY_LEFT_CONTROL,
	R_CTRL = GLFW_KEY_RIGHT_CONTROL,
	ENTER = GLFW_KEY_ENTER,
	ESC = GLFW_KEY_ESCAPE,
	LEFT = GLFW_KEY_LEFT,
	RIGHT = GLFW_KEY_RIGHT,
	UP = GLFW_KEY_UP,
	DOWN = GLFW_KEY_DOWN,
	// Add more as needed
};

enum class Button : int
{
	LEFT = GLFW_MOUSE_BUTTON_LEFT,
	RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
	MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
	// Add more as needed
};

/**
* Singleton class for receiving input from a window
*/
class Input
{
public:
	// Disable copy/move
	Input(const Input&) = delete;
	Input(Input&&) = delete;
	Input& operator=(const Input&) = delete;
	Input& operator=(Input&&) = delete;

	inline static void Init(GLFWwindow* window) {
		if (!Instance().m_Window)
		{
			Instance().m_Window = window;
		}
	};

	static bool IsKeyPressed(Key key) { return glfwGetKey(Instance().m_Window, (int)key); };
	static bool IsKeyClicked(Key key) 
	{ 
		auto& keys_pressed = Instance().m_KeysPressed;
		if (keys_pressed.find(key) != keys_pressed.end())
		{
			if (IsKeyPressed(key))
			{
				keys_pressed[key] = true;
				return false;
			}
			else if (keys_pressed[key] && !IsKeyPressed(key))
			{
				keys_pressed[key] = false;
				return true;
			}
		}
		else
		{
			keys_pressed.insert(std::make_pair(key, IsKeyPressed(key)));
		}
		return false;
	}
	static bool IsMousePressed(Button button) { return glfwGetMouseButton(Instance().m_Window, (int)button); };
	static bool IsMouseClicked(Button button) 
	{
		auto& buttons_pressed = Instance().m_ButtonsPressed;
		if (buttons_pressed.find(button) != buttons_pressed.end())
		{
			if (IsMousePressed(button))
			{
				buttons_pressed[button] = true;
				return false;
			}
			else if (buttons_pressed[button] && !IsMousePressed(button))
			{
				buttons_pressed[button] = false;
				return true;
			}
		}
		else
		{
			buttons_pressed.insert(std::make_pair(button, IsMousePressed(button)));
		}
		return false;
	}

	static void GetCursor(glm::dvec2& position) { glfwGetCursorPos(Instance().m_Window, &position.x, &position.y); };

private:
	// Don't allow public instantiation
	Input() = default;
	~Input() = default;

	static Input& Instance()
	{
		static Input i;
		return i;
	}

private:
	GLFWwindow* m_Window;

	std::map<Key, bool> m_KeysPressed;
	std::map<Button, bool> m_ButtonsPressed;
};

