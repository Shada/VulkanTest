#include "VulkanTestApplication.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location, 
	int32_t code, 
	const char* layerPrefix, 
	const char* msg, 
	void* userData) 
{
	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	bool isComplete()
	{
		return graphicsFamily >= 0;
	}
};

void HelloTriangleApplication::run() 
{
	this->initWindow();
	this->initVulkan();
	this->mainLoop();
}

void HelloTriangleApplication::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::initVulkan()
{
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
}

void HelloTriangleApplication::pickPhysicalDevice()
{

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if(deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for(const auto& device : devices)
	{
		if(isDeviceSuitable(device))
		{
			physicalDevice = device;
			break;
		}
	}
	if(physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU");
	}
}


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for(const auto& queueFamily : queueFamilies)
	{
		if(queueFamily.queueCount > 0 && queueFamily.queueFlags&VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}
		if(indices.isComplete())
		{
			break;
		}
		i++;
	}
	return indices;
}

void HelloTriangleApplication::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
	queueCreateInfo.queueCount = 1;

	float queueuPriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queueuPriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = 0;
	if(enableValidationLayers)
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}
	if(vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
}
bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = findQueueFamilies(device);

	return indices.isComplete() && deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		&& deviceFeatures.geometryShader;
}

void HelloTriangleApplication::mainLoop()
{
	while(!glfwWindowShouldClose(window))
	{
		
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
}

void HelloTriangleApplication::createInstance()
{
	if(enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo	= {};
	appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName	= "Hello Triangle";
	appInfo.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName			= "No Engine";
	appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion			= VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo		= &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount	= extensions.size();
	createInfo.ppEnabledExtensionNames	= extensions.data();

	if(enableValidationLayers)
	{
		createInfo.enabledLayerCount	= validationLayers.size();
		createInfo.ppEnabledLayerNames	= validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if(vkCreateInstance(&createInfo, nullptr, instance.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void HelloTriangleApplication::setupDebugCallback()
{
	if(!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if(CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback!");
	}
}

void HelloTriangleApplication::createSurface()
{
	if(glfwCreateWindowSurface(instance, window, nullptr, surface.replace()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for(unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	if(enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

bool HelloTriangleApplication::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for(const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for(const auto& layerProperties : availableLayers)
		{
			if(strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if(!layerFound)
		{
			return false;
		}
	}
	return true;
}