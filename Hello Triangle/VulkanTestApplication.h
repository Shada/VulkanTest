#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VDeleter.h"

const int WIDTH = 800;
const int HEIGHT = 600;

class VulkanTestApplication
{
public:
	void run();

private:
	void mainLoop();

	// Vulkan stuff
	void initVulkan();

	void createInstance();

	VDeleter<VkInstance> instance{ vkDestroyInstance };

	// GFLW stuff
	void initWindow();

	GLFWwindow *window;
	unsigned int glfwExtensionCount;
	const char** glfwExtensions;

};