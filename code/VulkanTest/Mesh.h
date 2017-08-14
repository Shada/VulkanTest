#pragma once

#include <vulkan\vulkan.hpp>
#include <tiny_obj_loader.h>
#include <string>
#include <map>

#include "VulkanHelpers.hpp"

#include "stdafx.h"
#include "Texture.h"

// TODO: Support various types of descriptors with different layouts. 


struct VulkanDescriptor
{
   VkDescriptorPool descriptorPool;
   VkDescriptorSetLayout descriptorSetLayout; // could be a vector.one for each descriptor set, or each descriptor points to a layout
   std::vector<VkDescriptorSet> descriptorSet;

   const VulkanStuff *vulkanStuff;

   VulkanDescriptor(const VulkanStuff *vulkanStuff)
   {
      this->vulkanStuff = vulkanStuff;
   }

   void createDescriptorPool()
   {
      std::array<VkDescriptorPoolSize, 3> poolSizes ={};
      poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      poolSizes[0].descriptorCount = 200;
      poolSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      poolSizes[1].descriptorCount = 200;
      poolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      poolSizes[2].descriptorCount = 200;

      VkDescriptorPoolCreateInfo poolInfo ={};
      poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
      poolInfo.pPoolSizes    = poolSizes.data();
      poolInfo.maxSets       = 200;

      if(vkCreateDescriptorPool(vulkanStuff->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to create descriptor pool!");
      }
   }

   int createDescriptorSet(VkDescriptorBufferInfo uniformBufferInfo, VkDescriptorBufferInfo dynamicBufferInfo, VkDescriptorImageInfo imageSamplerInfo)
   {
      VkDescriptorSet tempDescriptor;
      VkDescriptorSetLayout layouts[] ={ descriptorSetLayout };

      VkDescriptorSetAllocateInfo allocInfo ={};
      allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool     = descriptorPool;
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts        = layouts;

      if(vkAllocateDescriptorSets(vulkanStuff->device, &allocInfo, &tempDescriptor) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to allocate descriptor set!");
      }

      std::array<VkWriteDescriptorSet, 3> descriptorWrites ={};
      descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet           = tempDescriptor;
      descriptorWrites[0].dstBinding       = 0;
      descriptorWrites[0].dstArrayElement  = 0;
      descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount  = 1;
      descriptorWrites[0].pBufferInfo      = &uniformBufferInfo;
      descriptorWrites[0].pImageInfo       = nullptr;
      descriptorWrites[0].pTexelBufferView = nullptr;

      descriptorWrites[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet           = tempDescriptor;
      descriptorWrites[1].dstBinding       = 1;
      descriptorWrites[1].dstArrayElement  = 0;
      descriptorWrites[1].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      descriptorWrites[1].descriptorCount  = 1;
      descriptorWrites[1].pBufferInfo      = &dynamicBufferInfo;
      descriptorWrites[1].pImageInfo       = nullptr;
      descriptorWrites[1].pTexelBufferView = nullptr;

      descriptorWrites[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet           = tempDescriptor;
      descriptorWrites[2].dstBinding       = 2;
      descriptorWrites[2].dstArrayElement  = 0;
      descriptorWrites[2].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[2].descriptorCount  = 1;
      descriptorWrites[2].pBufferInfo      = nullptr;
      descriptorWrites[2].pImageInfo       = &imageSamplerInfo;
      descriptorWrites[2].pTexelBufferView = nullptr;

      vkUpdateDescriptorSets(vulkanStuff->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

      descriptorSet.push_back(tempDescriptor);

      return static_cast<uint32_t>(descriptorSet.size() - 1);
   }

   // This should be less hard coded. like send in an array of layout bindings
   void createDescriptorSetLayout()
   {
      VkDescriptorSetLayoutBinding uboLayoutBinding =
         vulkan::initialisers::createDescriptorSetLayoutBinding(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT);

      VkDescriptorSetLayoutBinding dynamicUboLayoutBinding =
         vulkan::initialisers::createDescriptorSetLayoutBinding(
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            VK_SHADER_STAGE_VERTEX_BIT);

      VkDescriptorSetLayoutBinding samplerLayoutBinding =
         vulkan::initialisers::createDescriptorSetLayoutBinding(
            2,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT);

      std::vector<VkDescriptorSetLayoutBinding> bindings =
      {
         uboLayoutBinding,
         dynamicUboLayoutBinding,
         samplerLayoutBinding
      };

      VkDescriptorSetLayoutCreateInfo layoutInfo ={};
      layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
      layoutInfo.pBindings    = bindings.data();

      if(vkCreateDescriptorSetLayout(vulkanStuff->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to create descriptor set layout!");
      }
   }

   void updateDescriptor(int index/* , insert data to be added to the descriptor here */)
   {
      // update the descriptor at index position
   }

   const VkDescriptorSet *getDescriptorSet(int index)
   {
      return &descriptorSet[index];
   }
};

/// TODO: Add unique name identifier string? Should be able to get mesh stuff by unique name
/// numerize name? ex: "model", "model1", "model2" ?
/// when adding name, returning name or id..? 
/// should be able so search for meshobject in some console or something


// TODO: enable more that one model per buffer.
// a vertex buffer should have some minimal amount of vertices
// this optimizes things when having low poly meshes.

// when drawing each buffer should only be loaded once.
// use offsets when drawing.
// need to utilize some class or stucture that couples a mesh with a worldObject

// will have to load the material somehow. one material per submesh. 
// dynamic buffer? push constants? have to research best practices.

class Mesh
{
private:
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
      int32_t meshId = -1; // not needed, but might keep it for now.
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

public:
   Mesh(const VulkanStuff* vulkanStuff);
   ~Mesh();

   // TODO: do so we can send in one or more objects? currently only one at a time
   void loadMesh(const char* fileNames);

   void draw(int commandBufferIndex);

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

   std::vector<SubMesh> getSubMeshesForMesh(uint32_t meshId)
   {
      return subMeshMap[meshId];
   }

   uint32_t getNumberOfMeshes()
   {
      return numberOfMeshes;
   }

private:

   void createVertexBuffer();
   void createIndexBuffer();


   const VulkanStuff* vulkanStuff;

   std::vector<VertexData> vertexData;

   std::vector<Material> material;


   // TODO: create struct/class that handles buffers. All types of buffers. 
   std::vector<VkBuffer> vertexBuffer;
   std::vector<VkDeviceMemory> vertexBufferMemory;
   std::vector<VkBuffer> indexBuffer;
   std::vector<VkDeviceMemory> indexBufferMemory;

   std::map<int, std::vector<SubMesh>> subMeshMap;

   uint32_t numberOfMeshes = 0;

   VulkanDescriptor *descriptor;

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