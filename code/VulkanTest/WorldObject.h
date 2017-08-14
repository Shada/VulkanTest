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

   void invalidateModelMatrix(uint32_t index);
   void updateModelMatrix();

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
   VkBuffer worldMatrixUBO;
   VkDeviceMemory worldMatrixUBOMemory;

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

   void updateDescriptorSet();
public:
   void createDescriptorSetLayout();
   void createDescriptorPool();
   void createDescriptorSet();

   VkDescriptorSetLayout getDescriptorSetLayout()
   {
      return descriptorSetLayout;
   }
   VkDescriptorSet getDescriptorSet()
   {
      return descriptorSet;
   }
};

