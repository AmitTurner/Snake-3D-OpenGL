#pragma once   //maybe should be static class
#include "display.h"
#include "game.h"


void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		Game *scn = (Game*)glfwGetWindowUserPointer(window);
		double x2, y2;
		glfwGetCursorPos(window, &x2, &y2);
		scn->picking((int)x2, (int)y2);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Game *scn = (Game*)glfwGetWindowUserPointer(window);
	scn->shapeTransformation(scn->zCameraTranslate, (float)yoffset);
}

vec3 x(1, 0, 0);
vec3 y(0, 1, 0);
vec3 z(0, 0, 1);
bool stateActive = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Game *scn = (Game*)glfwGetWindowUserPointer(window);
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
			case GLFW_KEY_ESCAPE:
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
			case GLFW_KEY_P:
				scn->changeMode();
			break;
			case GLFW_KEY_F1:
				scn->loadNextLevel();
			break;
			case GLFW_KEY_F2:
				scn->switchMouseEnablePicking();
			break;
			case GLFW_KEY_F3:
				scn->resetSnake();
			break;
			case GLFW_KEY_D:
				scn->changeCameraMode();
			break;
			case GLFW_KEY_RIGHT://TODO head intersect with nodes disable rotation
				scn->playerInput(true);
			break;
			case GLFW_KEY_LEFT:
				scn->playerInput(false);
			break;
			case GLFW_KEY_KP_SUBTRACT:
				scn->shapeTransformation(scn->zCameraTranslate, -50.f);
			break;
			case GLFW_KEY_KP_ADD:
				scn->shapeTransformation(scn->zCameraTranslate, 50.f);
			break;
			case GLFW_KEY_SPACE:
				scn->PauseUnpause();
			break;
			case GLFW_KEY_GRAVE_ACCENT:
				scn->Debug();
			break;
			default:
				printf("undefined key %d\n", key);
			break;
		}
	}
}


void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	Game *scn = (Game*)glfwGetWindowUserPointer(window);

	scn->updatePosition((float)xpos, (float)ypos);
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		scn->mouseProccessing(GLFW_MOUSE_BUTTON_RIGHT);
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		scn->mouseProccessing(GLFW_MOUSE_BUTTON_LEFT);
	}

}

void window_size_callback(GLFWwindow* window, int width, int height)
{
	Game *scn = (Game*)glfwGetWindowUserPointer(window);

	scn->resize(width, height);
	//relation = (float)width/(float)height;
}

void init(Display &display)
{
	display.addKeyCallBack(key_callback);
	display.addMouseCallBacks(mouse_callback, scroll_callback, cursor_position_callback);
	display.addResizeCallBack(window_size_callback);

}
