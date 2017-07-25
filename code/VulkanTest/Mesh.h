#pragma once
#include <vulkan\vulkan.hpp>
#include <vector>
#include <glm\glm.hpp>
#include "stdafx.h"
#include <string>

/// TODO: Add unique name identifier string? Should be able to get mesh stuff by unique name
/// numerize name? ex: "model", "model1", "model2" ?
/// when adding name, returning name or id..? 
/// should be able so search for meshobject in some console or something

class Mesh
{
public:
   Mesh(const VulkanStuff* vulkanStuff);
   ~Mesh();

   // TODO: do so we can send in one or more objects? currently only one at a time
   void loadMesh(const char* fileNames); 

   VkBuffer getVertexBuffer(int index)
   {
      return vertexBuffer[index];
   }
   VkBuffer getIndexBuffer(int index)
   {
      return indexBuffer[index];
   }
   uint32_t getNumIndices(int index)
   {
      return static_cast<uint32_t>(vertexData[index].indices.size());
   }

   uint32_t getNumberOfMeshes()
   {
      return static_cast<uint32_t>(modelName.size());
   }

private:
   
   void createVertexBuffer();
   void createIndexBuffer();

   struct VertexData
   {
      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;
   };

   const VulkanStuff* vulkanStuff;

   std::vector<VertexData> vertexData;

   std::vector<VkBuffer> vertexBuffer;
   std::vector<VkDeviceMemory> vertexBufferMemory;
   std::vector<VkBuffer> indexBuffer;
   std::vector<VkDeviceMemory> indexBufferMemory;
   std::vector<std::string> modelName;

   Vertex extractVertexFromAttrib(tinyobj::attrib_t attrib, const tinyobj::index_t index);

// move to some helper_class
   void createBuffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer *buffer,
      VkDeviceMemory *bufferMemory);

   void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
   {
      VkCommandBuffer commandBuffer = beginSingleTimeCommand();

      VkBufferCopy bufferCopy ={};
      bufferCopy.srcOffset = 0;
      bufferCopy.dstOffset = 0;
      bufferCopy.size      = size;

      vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

      endSingleTimeCommand(commandBuffer);
   }

   VkCommandBuffer beginSingleTimeCommand()
   {
      VkCommandBufferAllocateInfo allocInfo ={};
      allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandPool        = vulkanStuff->commandPool;
      allocInfo.commandBufferCount = 1;

      VkCommandBuffer commandBuffer;
      vkAllocateCommandBuffers(vulkanStuff->device, &allocInfo, &commandBuffer);

      VkCommandBufferBeginInfo beginInfo ={};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      vkBeginCommandBuffer(commandBuffer, &beginInfo);

      return commandBuffer;
   }

   void endSingleTimeCommand(VkCommandBuffer commandBuffer)
   {
      vkEndCommandBuffer(commandBuffer);

      VkSubmitInfo submitInfo ={};
      submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers    = &commandBuffer;

      vkQueueSubmit(vulkanStuff->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

      vkQueueWaitIdle(vulkanStuff->graphicsQueue);

      vkFreeCommandBuffers(vulkanStuff->device, vulkanStuff->commandPool, 1, &commandBuffer);
   }
   uint32_t findMemoryType(uint32_t typeFiter, VkMemoryPropertyFlags properties)
   {
      VkPhysicalDeviceMemoryProperties memoryProperties;
      vkGetPhysicalDeviceMemoryProperties(vulkanStuff->physicalDevice, &memoryProperties);

      for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
      {
         if(typeFiter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags&properties) == properties)
         {
            return i;
         }
      }

      throw std::runtime_error("failed to find suitable memory type!");
   }
};