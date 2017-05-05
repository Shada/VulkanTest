#pragma once
#include "stdafx.h"

class HelloTriangleApplication
{
public:
	void run();

private:
	GLFWwindow* window;

	VDeleter<VkInstance> instance{ vkDestroyInstance };

	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue;

	VDeleter<VkDevice> device{ vkDestroyDevice };

	VDeleter<VkSurfaceKHR> surface{ instance,vkDestroySurfaceKHR };

	void initWindow();

	void initVulkan();

	void mainLoop();

	void createInstance();

	void setupDebugCallback();

	void createSurface();

	void pickPhysicalDevice();

	void createLogicalDevice();

	std::vector<const char*> getRequiredExtensions();

	bool checkValidationLayerSupport();

	bool isDeviceSuitable(VkPhysicalDevice);
};