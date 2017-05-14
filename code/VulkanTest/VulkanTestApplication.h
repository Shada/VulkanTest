#pragma once
#include "stdafx.h"
#include "VulkanShader.h"

class HelloTriangleApplication
{
private:
	// structs
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentFamily = -1;
		bool isComplete()
		{
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

public:
	void run();

private:

	void mainLoop();

	// glfw stuff
	GLFWwindow* window;
	void initWindow();

	// vulkan stuff
	VDeleter<VkInstance> instance{ vkDestroyInstance };

	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VDeleter<VkDevice> device{ vkDestroyDevice };

	VDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };

	VulkanShader vertShader;
	VulkanShader fragShader;

	void initVulkan();

	void createInstance();

	void setupDebugCallback();

	void createSurface();

	void pickPhysicalDevice();

	void createLogicalDevice();

	void createGraphicsPipeline();

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);

	std::vector<const char*> getRequiredExtensions();

	bool checkValidationLayerSupport();

	bool isDeviceSuitable(VkPhysicalDevice);

	bool checkDeviceExtensionSupport(VkPhysicalDevice);

	//swap chain stuff
	VDeleter<VkSwapchainKHR> swapChain{ device, vkDestroySwapchainKHR };
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	void createSwapChain();

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);

	//image views
	std::vector<VDeleter<VkImageView>> swapChainImageViews;

	void createImageViews();
};