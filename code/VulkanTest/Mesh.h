#pragma once
#include <vulkan\vulkan.hpp>
#include <vector>
#include <glm\glm.hpp>
#include "stdafx.h"
#include <string>

/// TODO: Add unique name identifier. Should be able to get mesh stuff by unique name
/// numerize name? ex: "model", "model1", "model2" ?
/// when adding name, returning name or id..? 
/// should be able so search for meshobject in some console or something

/// TODO: split the mesh and the object. Mesh stuff like vetices and buffers in one structure /class
/// and meshobject that references a mesh with the id, the meshobject have the transformations and
/// and the behaviour. Coud be different types of objects too, like static/dynamic/animated etc
class Mesh
{
public:
   Mesh(const VulkanStuff* vulkanStuff);
   ~Mesh();

   // TODO: do so we can send in one or more objects. currently only one at a time
   void loadMesh(const char* fileNames); 

   // might want to send in initialising stuff.. 
   // returns the index 
   // could also do it by name?
   int addInstance(int meshId); 

   // this one updates all the models
   void update(float dt);

   void setPosition(glm::vec3, int modelindex);
   void setRotation(glm::vec3, int modelindex);
   void setScale(glm::vec3, int modelindex);

   void setRotationSpeed(float yaw, float pitch, float roll, int index);
   void setMovingSpeed(float movingSpeed, int index);
   void setMovingDirection(glm::vec3 movingDirection, int index);

   VkBuffer getVertexBuffer(int index)
   {
      size_t bufferIndex = objectData.bufferId[index];
      return vertexBuffer[bufferIndex];
   }
   VkBuffer getIndexBuffer(int index)
   {
      size_t bufferIndex = objectData.bufferId[index];
      return indexBuffer[bufferIndex];
   }
   uint32_t getNumIndices(int index)
   {
      return static_cast<uint32_t>(vertexData[index].indices.size());
   }

   glm::mat4 getModelMatrix(int index)
   {
      return objectData.modelMatrix[index];
   }

   uint32_t getNumObjects()
   {
      return static_cast<uint32_t>(objectData.bufferId.size());
   }

private:
   
   void invalidateModelMatrix(size_t modelIndex);
   void updateModelMatrix();

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

   struct ObjectData
   {
      std::vector<size_t> bufferId;
      std::vector<glm::vec3> position;
      std::vector<glm::vec3> rotation;
      std::vector<glm::vec3> scale;

      std::vector<glm::mat4> modelMatrix;
      std::vector<bool> invalidModelMatrix;

      // these are only for movable objects.
      // should maybe break out to another object
      std::vector<glm::vec3> movingDirection;
      std::vector<float> movingSpeed;
      std::vector<glm::vec3> rotationSpeed;

      // When setting pos/rot/scale
      // we use these to make nice transitions
      std::vector<glm::vec3> targetScale;
      std::vector<bool> isChangingScale;
      std::vector<glm::vec3> targetPosition;
      std::vector<bool> isChangingPosition;
      std::vector<glm::vec3> targetRotation;
      std::vector<bool> isChangingRotation;
   }objectData;
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