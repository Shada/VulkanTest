#pragma once

#include <vulkan\vulkan.hpp>
#include <tiny_obj_loader.h>
#include <string>

#include "stdafx.h"
#include "Texture.h"

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

   struct SubMesh
   {
      int32_t startIndex = -1;
      int32_t numberOfIndices = 0;
      int32_t materialId = -1;
      int32_t meshId = -1;
      int32_t descriptorSetId = -1;
   };

   struct Material
   {
      int32_t diffuseTextureId = -1;
      int32_t specularTextureId = -1;
      int32_t bumpTextureId = -1;
      glm::vec3 diffuseColour;
      glm::vec3 specularColour;
      glm::vec3 ambientColour;
   };

   const VulkanStuff* vulkanStuff;

   std::vector<VertexData> vertexData;

   std::vector<Material> material;

   // TODO: create struct/class that handles buffers. All types of buffers. 
   std::vector<VkBuffer> vertexBuffer;
   std::vector<VkDeviceMemory> vertexBufferMemory;
   std::vector<VkBuffer> indexBuffer;
   std::vector<VkDeviceMemory> indexBufferMemory;

   std::vector<SubMesh> subMesh;

   std::vector<VkDescriptorSet> descriptorSet;

   // TODO: might need a vector of these to make sure that I can expand with more descriptors if needed.
   // for example have a decriptorPool for up to ~100 meshes, and then create a new descriptorPool if it's needed.
   VkDescriptorPool descriptorPool;

   Texture *texture;

   std::vector<std::string> modelName;

   inline void loadMaterials(std::vector<tinyobj::material_t>& materials);

   inline void extractVertexFromAttrib(Vertex& vertex, tinyobj::attrib_t& attrib, tinyobj::index_t& index);

// TODO: Should be in Buffer class
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

   // TODO: helper class? or some other place? Not here.
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

struct VulkanDescriptors
{
   VkDescriptorPool descriptorPool;
   std::vector<VkDescriptorSet> descriptorSet;

   void createDescriptorPool()
   {
      // copy create code here
   }

   int createDescriptor()
   {
      // add create code here.
      // returns the Id of the descriptor so it can be updated etc
      return 0;
   }

   void updateDescriptor(int index/* , insert data to be added to the descriptor here */)
   {
      // update the descriptor at index position
   }

   VkDescriptorSet getDescriptorSet(int index)
   {
      return descriptorSet[index];
   }
};