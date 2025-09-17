#include "Window.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>

constexpr int ScreenWidth = 1280;
constexpr int ScreenHeight = 720;

void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);



int main()
{

}
