#pragma once

#include <iostream>

#include "vulkan\vulkan.h"

struct VulkanDevice
{
   VkPhysicalDevice physicalDevice;

   VkDevice device;

   VkInstance instance;

   VkPhysicalDeviceProperties deviceProperties;

   VkPhysicalDeviceFeatures deviceFeatures;

   VkCommandPool commandPool = VK_NULL_HANDLE;

   void createBuffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer *buffer,
      VkDeviceMemory *bufferMemory)
   {
      // use vulkan::initialisers::bufferCreateInfo()
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
      allocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);

      if(vkAllocateMemory(device, &allocateInfo, nullptr, bufferMemory) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to allocate buffer memory!");
      }

      if(vkBindBufferMemory(device, *buffer, *bufferMemory, 0) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to bind buffer memory!");
      }
   }

   uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFiter, VkMemoryPropertyFlags properties)
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

   void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue queue)
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

   void endSingleTimeCommand(VkCommandBuffer commandBuffer, VkQueue queue)
   {
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