#pragma once

#include <iostream>
#include <set>

#include "vulkan\vulkan.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace vks
{
   const std::vector<const char*> deviceExtensions =
   {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
   };

   const std::vector<const char*> validationLayers =
   {
      "VK_LAYER_LUNARG_standard_validation"
   };

   struct QueueFamilyIndices
   {
      int graphicsFamily = -1;
      int presentFamily = -1;
      bool isComplete()
      {
         return graphicsFamily >= 0 && presentFamily >= 0;
      }
   };

   struct VulkanDevice
   {
      VkPhysicalDevice physicalDevice;

      VkDevice device;

      // TODO: find out best practice of how to handle instance...
      VkInstance instance;

      VkSurfaceKHR surface;

      VkPhysicalDeviceProperties deviceProperties;

      VkPhysicalDeviceFeatures deviceFeatures;

      VkCommandPool commandPool = VK_NULL_HANDLE;

      VkQueue graphicsQueue;
      VkQueue presentQueue;

      void createLogicalDevice()
      {
         QueueFamilyIndices indices = findQueueFamilies();

         std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
         std::set<int> uniqueQueueFamilies =
         {
            indices.graphicsFamily,
            indices.presentFamily
         };

         float queuePriority = 1.0f;
         for(int queueFamily : uniqueQueueFamilies)
         {
            VkDeviceQueueCreateInfo queueCreateInfo ={};

            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount       = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
         }

         VkPhysicalDeviceFeatures deviceFeatures ={};
         deviceFeatures.samplerAnisotropy = VK_TRUE;

         VkDeviceCreateInfo createInfo ={};
         createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
         createInfo.pQueueCreateInfos       = queueCreateInfos.data();
         createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
         createInfo.pEnabledFeatures        = &deviceFeatures;
         createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
         createInfo.ppEnabledExtensionNames = deviceExtensions.data();

         if(enableValidationLayers)
         {
            createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
         }
         else
         {
            createInfo.enabledLayerCount = 0;
         }

         if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
         {
            throw std::runtime_error("failed to create logical device");
         }

         // TODO: here? not sure if I want the queues here or somewhere else... I'll keep them heere for now. 
         vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
         vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
      }

      QueueFamilyIndices findQueueFamilies()
      {
         QueueFamilyIndices indices;

         uint32_t queueFamilyCount = 0;
         vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

         std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
         vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

         VkBool32 presentSupport = false;
         int i = 0;

         for(const auto& queueFamily : queueFamilies)
         {
            // this should be moved too code related to swapchain, creation of present surface.
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
               indices.graphicsFamily = i;
            }
            if(queueFamily.queueCount > 0 && presentSupport)
            {
               indices.presentFamily = i;
            }
            if(indices.isComplete())
            {
               break;
            }
            i++;
         }

         return indices;
      }

      void createBuffer(
         VkDeviceSize size,
         VkBufferUsageFlags usage,
         VkMemoryPropertyFlags properties,
         VkBuffer *buffer,
         VkDeviceMemory *bufferMemory)
      {
         // use vkn::inits::bufferCreateInfo()
         VkBufferCreateInfo bufferInfo ={};
         bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
         bufferInfo.size        = size;
         bufferInfo.usage       = usage;
         bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

         if(vkCreateBuffer(device, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
         {
            throw std::runtime_error("failed to create buffer!");
         }

         VkMemoryRequirements memoryRequirements;
         vkGetBufferMemoryRequirements(device, *buffer, &memoryRequirements);

         VkMemoryAllocateInfo allocateInfo ={};
         allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
         allocateInfo.allocationSize  = memoryRequirements.size;
         allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

         if(vkAllocateMemory(device, &allocateInfo, nullptr, bufferMemory) != VK_SUCCESS)
         {
            throw std::runtime_error("failed to allocate buffer memory!");
         }

         if(vkBindBufferMemory(device, *buffer, *bufferMemory, 0) != VK_SUCCESS)
         {
            throw std::runtime_error("failed to bind buffer memory!");
         }
      }

      uint32_t findMemoryType(uint32_t typeFiter, VkMemoryPropertyFlags properties)
      {
         VkPhysicalDeviceMemoryProperties memoryProperties;
         vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

         for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
         {
            if(typeFiter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags&properties) == properties)
            {
               return i;
            }
         }

         throw std::runtime_error("failed to find suitable memory type!");
      }

      void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue = nullptr)
      {
         VkCommandBuffer commandBuffer = beginSingleTimeCommand();

         VkBufferCopy bufferCopy ={};
         bufferCopy.srcOffset = 0;
         bufferCopy.dstOffset = 0;
         bufferCopy.size      = size;

         vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

         endSingleTimeCommand(commandBuffer, queue);
      }

      VkCommandBuffer beginSingleTimeCommand()
      {
         VkCommandBufferAllocateInfo allocInfo ={};
         allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
         allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
         allocInfo.commandPool        = commandPool;
         allocInfo.commandBufferCount = 1;

         VkCommandBuffer commandBuffer;
         vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

         VkCommandBufferBeginInfo beginInfo ={};
         beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
         beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

         vkBeginCommandBuffer(commandBuffer, &beginInfo);

         return commandBuffer;
      }

      void endSingleTimeCommand(VkCommandBuffer commandBuffer, VkQueue queue = nullptr)
      {
         if(queue == nullptr) queue = graphicsQueue;

         vkEndCommandBuffer(commandBuffer);

         VkSubmitInfo submitInfo ={};
         submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
         submitInfo.commandBufferCount = 1;
         submitInfo.pCommandBuffers    = &commandBuffer;

         vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

         vkQueueWaitIdle(queue);

         vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
      }
   };
};