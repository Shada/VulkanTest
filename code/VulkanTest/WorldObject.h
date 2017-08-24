#pragma once

#include <vector>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\glm.hpp>

//TODO: don't want to include this... so probably should move VulkanStuff
#include "stdafx.h"

#include "WorldObjectToMeshMapper.h"
#include "VulkanHelpers.hpp"



class WorldObject
{
public:

   WorldObject(WorldObjectToMeshMapper* worldObjectToMeshMapper, const VulkanStuff *vulkanStuff);
   ~WorldObject();

   // could also do it by name?
   uint32_t addInstance(uint32_t meshId);
   uint32_t addInstance(uint32_t meshId, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);

   void update(float dt);

   void setPosition(uint32_t index, glm::vec3 position);
   void setRotation(uint32_t index, glm::vec3 rotation);
   void setScale(uint32_t index, glm::vec3 scale);

   void setRotationSpeed(uint32_t index, float yaw, float pitch, float roll);
   void setMovingSpeed(uint32_t index, float movingSpeed);
   void setMovingDirection(uint32_t index, glm::vec3 movingDirection);

   glm::mat4 getModelMatrix(uint32_t index)
   {
      return modelMatrix[index];
   }

   uint32_t getNumberOfObjects()
   {
      return numberOfObjects;
   }

   uint32_t getMeshId(uint32_t index)
   {
      return meshId[index];
   }

private:

   size_t dynamicBufferSize = 0;
   struct UboDataDynamic
   {
      glm::mat4 *model = nullptr;
   } uboDataDynamic;


   void* alignedAlloc(size_t size, size_t alignment)
   {
      void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
      data = _aligned_malloc(size, alignment);
#else 
      int res = posix_memalign(&data, alignment, size);
      if(res != 0)
         data = nullptr;
#endif
      return data;
   }

   void alignedFree(void* data)
   {
#if	defined(_MSC_VER) || defined(__MINGW32__)
      _aligned_free(data);
#else 
      free(data);
#endif
   }

   struct
   {
      VkBuffer buffer;
      VkDeviceMemory memory;
   } worldMatrixUBO;

   float animationTimer = 0.0f;

   size_t dynamicAlignment;

   void invalidateModelMatrix(uint32_t index);
   void updateModelMatrix();

   void createUniformBuffer();

   WorldObjectToMeshMapper* worldObjectToMeshMapper;
   const VulkanStuff* vulkanStuff;

   uint32_t numberOfObjects;
   std::vector<uint32_t> meshId;
   std::vector<glm::vec3> position;
   std::vector<glm::vec3> rotation;
   std::vector<glm::vec3> scale;

   std::vector<glm::mat4> modelMatrix;
   std::vector<bool> isModelMatrixInvalid;

   VkDescriptorPool descriptorPool;
   VkDescriptorSetLayout descriptorSetLayout;
   VkDescriptorSet descriptorSet;

   // these are only for movable objects.
   // should maybe break out static objects to another class
   std::vector<glm::vec3> movingDirection;
   std::vector<float> movingSpeed;
   std::vector<glm::vec3> rotationSpeed;

   // When setting pos/rot/scale
   // we use these to make nice transitions ? not yet really implemented
   std::vector<glm::vec3> targetScale;
   std::vector<bool> isChangingScale;
   std::vector<glm::vec3> targetPosition;
   std::vector<bool> isChangingPosition;
   std::vector<glm::vec3> targetRotation;
   std::vector<bool> isChangingRotation;

   VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding()
   {
      return vulkan::initialisers::createDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
   }

   void createBuffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer *buffer,
      VkDeviceMemory *bufferMemory)
   {
      VkBufferCreateInfo bufferInfo ={};
      bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size        = size;
      bufferInfo.usage       = usage;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if(vkCreateBuffer(vulkanStuff->device, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to create vertex buffer!");
      }

      VkMemoryRequirements memoryRequirements;
      vkGetBufferMemoryRequirements(vulkanStuff->device, *buffer, &memoryRequirements);

      VkMemoryAllocateInfo allocateInfo ={};
      allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocateInfo.allocationSize  = memoryRequirements.size;
      allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

      if(vkAllocateMemory(vulkanStuff->device, &allocateInfo, nullptr, bufferMemory) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to allocate vertex buffer memory!");
      }

      vkBindBufferMemory(vulkanStuff->device, *buffer, *bufferMemory, 0);
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


public:
   void createDescriptorSetLayout();
   void createDescriptorPool();
   void createDescriptorSet();

   // TODO call this from add instance ? maybe using a boolean to say if it shall update?
   void updateDescriptorSet();

   void updateDynamicUniformBuffer();

   VkDescriptorSetLayout getDescriptorSetLayout()
   {
      return descriptorSetLayout;
   }
   VkDescriptorSet *getDescriptorSet()
   {
      return &descriptorSet;
   }
   size_t getDynamicAlignment()
   {
      return dynamicAlignment;
   }
};

