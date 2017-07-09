#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#include <GLFW/glfw3.h>
#include "VDeleter.h"

#include <vector>
#include <iostream>
#include <array>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtx\hash.hpp>

#include <chrono>

const std::string MODEL_PATH = "models/chalet.obj";
const std::string TEXTURE_PATH = "textures/chalet.jpg";

// somewhere else.. Vulkan_helper_stuff.h?
struct VulkanStuff
{
   VDeleter<VkInstance> instance{ vkDestroyInstance };
   VkPhysicalDevice physicalDevice;
   VkQueue graphicsQueue;
   VkQueue presentQueue;
   VDeleter<VkDevice> device{ vkDestroyDevice };
   VDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };
   VDeleter<VkCommandPool> commandPool{ device, vkDestroyCommandPool };
};

struct Vertex
{
   glm::vec3 position;
   glm::vec3 colour;
   glm::vec2 texCoord;

   static VkVertexInputBindingDescription getBindingDescription()
   {
      VkVertexInputBindingDescription bindingDescription ={};

      bindingDescription.binding   = 0;
      bindingDescription.stride    = sizeof(Vertex);
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      return bindingDescription;
   }
   static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
   {
      std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions ={};
      attributeDescriptions[0].binding  = 0;
      attributeDescriptions[0].location = 0;
      attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[0].offset   = offsetof(Vertex, position);

      attributeDescriptions[1].binding  = 0;
      attributeDescriptions[1].location = 1;
      attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[1].offset   = offsetof(Vertex, colour);

      attributeDescriptions[2].binding  = 0;
      attributeDescriptions[2].location = 2;
      attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
      attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

      return attributeDescriptions;
   }

   bool operator==(const Vertex& other) const
   {
      return position == other.position && 
         texCoord == other.texCoord &&
         colour == other.colour;
   }
};

namespace std
{
   template<> struct hash<Vertex>
   {
      size_t operator()(Vertex const& vertex) const
      {
         return ((hash<glm::vec3>()(vertex.position) ^
            (hash<glm::vec3>()(vertex.colour) << 1)) >> 1) ^
            (hash<glm::vec2>()(vertex.texCoord) << 1);
      }
   };
}

struct UniformBufferObject
{
   glm::mat4 model;
   glm::mat4 view;
   glm::mat4 proj;
};

const std::vector<const char*> validationLayers =
{
   "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = 
{
   VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

inline VkResult CreateDebugReportCallbackEXT(
   VkInstance instance,
   const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
   const VkAllocationCallbacks* pAllocator,
   VkDebugReportCallbackEXT* pCallback)
{
   auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

   if(func != nullptr)
   {
      return func(instance, pCreateInfo, pAllocator, pCallback);
   }
   else
   {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   }
}

inline void DestroyDebugReportCallbackEXT(
   VkInstance instance,
   VkDebugReportCallbackEXT callback,
   const VkAllocationCallbacks* pAllocator)
{
   auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

   if(func != nullptr)
   {
      func(instance, callback, pAllocator);
   }
}

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int WIDTH  = 800;
const int HEIGHT = 600;