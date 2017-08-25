#include "WorldObject.h"


//TODO: add collision box and collision detections. Object vs object, and object vs ray to start


WorldObject::WorldObject(WorldObjectToMeshMapper *worldObjectToMeshMapper, vks::VulkanDevice* vulkanDevice)
{
   this->worldObjectToMeshMapper = worldObjectToMeshMapper;

   this->vulkanDevice = vulkanDevice;

   numberOfObjects = 0;
}

WorldObject::~WorldObject()
{
   vkDestroyBuffer(vulkanDevice->device, worldMatrixUBO.buffer, nullptr);
   vkFreeMemory(vulkanDevice->device, worldMatrixUBO.memory, nullptr);

   vkDestroyDescriptorSetLayout(vulkanDevice->device, descriptorSetLayout, nullptr);
}

void WorldObject::update(float dt)
{
   for(uint32_t i = 0; i < position.size(); i++)
   {
      if(movingSpeed[i] != 0.f)
      {
         position[i] += dt * movingSpeed[i] * movingDirection[i];
         invalidateModelMatrix(i);
      }
   }
   for(uint32_t i = 0; i < rotation.size(); i++)
   {
      if(rotationSpeed[i] != glm::vec3(0.f))
      {
         rotation[i] += dt * rotationSpeed[i];
         invalidateModelMatrix(i);
      }
   }

   updateModelMatrix();
}

void WorldObject::updateModelMatrix()
{
   for(size_t i = 0; i < modelMatrix.size(); i++)
   {
      if(isModelMatrixInvalid[i])
      {
         modelMatrix[i] = glm::translate(glm::mat4(), position[i]);

         modelMatrix[i] = glm::rotate(modelMatrix[i], glm::radians(rotation[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
         modelMatrix[i] = glm::rotate(modelMatrix[i], glm::radians(rotation[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
         modelMatrix[i] = glm::rotate(modelMatrix[i], glm::radians(rotation[i].x), glm::vec3(1.0f, 0.0f, 0.0f));

         modelMatrix[i] = glm::scale(modelMatrix[i], scale[i]);
      }
   }

   updateDynamicUniformBuffer();
}

void WorldObject::createDescriptorPool()
{
   VkDescriptorPoolSize poolSize ={};
   poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   poolSize.descriptorCount = 1;

   VkDescriptorPoolCreateInfo poolInfo ={};
   poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = 1;
   poolInfo.pPoolSizes    = &poolSize;
   poolInfo.maxSets       = 1;

   if(vkCreateDescriptorPool(vulkanDevice->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create descriptor pool!");
   }
}

void WorldObject::createDescriptorSetLayout()
{
   auto descriptorSetLayoutBinding = createDescriptorSetLayoutBinding();

   auto layoutCreateInfo = vulkan::initialisers::createDescriptorSetLayoutCreateInfo(descriptorSetLayoutBinding);

   if(vkCreateDescriptorSetLayout(vulkanDevice->device, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create Matrix Buffer descriptor set layout!");
   }
}

void WorldObject::createDescriptorSet()
{
   // TODO: not the responsibility of this function to do this. 
   // We should however have a check that makes sure that UBO and descriptor pool is initialized
   createUniformBuffer();
   createDescriptorPool();

   VkDescriptorSetAllocateInfo allocInfoMatrixBuffer ={};
   allocInfoMatrixBuffer.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfoMatrixBuffer.descriptorPool     = descriptorPool;
   allocInfoMatrixBuffer.descriptorSetCount = 1;
   allocInfoMatrixBuffer.pSetLayouts        = &descriptorSetLayout;

   if(vkAllocateDescriptorSets(vulkanDevice->device, &allocInfoMatrixBuffer, &descriptorSet) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate MatrixBuffer descriptor set!");
   }


   VkDescriptorBufferInfo dynamicBufferInfo ={};
   dynamicBufferInfo.buffer = worldMatrixUBO.buffer;
   dynamicBufferInfo.offset = 0;
   dynamicBufferInfo.range  = VK_WHOLE_SIZE;

   VkWriteDescriptorSet descriptorWritesMatrixBuffer ={};

   descriptorWritesMatrixBuffer.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesMatrixBuffer.dstSet           = descriptorSet;
   descriptorWritesMatrixBuffer.dstBinding       = 1;
   descriptorWritesMatrixBuffer.dstArrayElement  = 0;
   descriptorWritesMatrixBuffer.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   descriptorWritesMatrixBuffer.descriptorCount  = 1;
   descriptorWritesMatrixBuffer.pBufferInfo      = &dynamicBufferInfo;
   descriptorWritesMatrixBuffer.pImageInfo       = nullptr;
   descriptorWritesMatrixBuffer.pTexelBufferView = nullptr;

   vkUpdateDescriptorSets(vulkanDevice->device, 1, &descriptorWritesMatrixBuffer, 0, nullptr);
}

uint32_t WorldObject::addInstance(uint32_t meshId)
{
   this->meshId.push_back(meshId);
   position.push_back(glm::vec3(0.f));
   rotation.push_back(glm::vec3(0.f));
   scale.push_back(glm::vec3(1.f));
   rotationSpeed.push_back(glm::vec3(0.f, 0.f, 10.f));
   movingSpeed.push_back(0.f);
   isModelMatrixInvalid.push_back(true);
   isChangingPosition.push_back(false);
   isChangingRotation.push_back(false);
   isChangingScale.push_back(false);
   modelMatrix.push_back(glm::mat4());
   movingDirection.push_back(glm::vec3(0.f));
   targetPosition.push_back(glm::vec3(0.f));
   targetRotation.push_back(glm::vec3(0.f));
   targetScale.push_back(glm::vec3(0.f));

   worldObjectToMeshMapper->addWorldObject(meshId, numberOfObjects);

   return static_cast<uint32_t>(numberOfObjects++);
}

uint32_t WorldObject::addInstance(uint32_t meshId, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
   this->meshId.push_back(meshId);
   this->position.push_back(position);
   this->rotation.push_back(rotation);
   this->scale.push_back(scale);
   rotationSpeed.push_back(glm::vec3(0.f, 0.f, 10.f));
   movingSpeed.push_back(0.f);
   isModelMatrixInvalid.push_back(true);
   isChangingPosition.push_back(false);
   isChangingRotation.push_back(false);
   isChangingScale.push_back(false);
   modelMatrix.push_back(glm::mat4());
   movingDirection.push_back(glm::vec3(0.f));
   targetPosition.push_back(glm::vec3(0.f));
   targetRotation.push_back(glm::vec3(0.f));
   targetScale.push_back(glm::vec3(0.f));

   worldObjectToMeshMapper->addWorldObject(meshId, numberOfObjects);

   return static_cast<uint32_t>(numberOfObjects++);
}


void WorldObject::setPosition(uint32_t index, glm::vec3 position)
{
   targetPosition[index] = position;
   isChangingPosition[index] = true;
}

void WorldObject::setRotation(uint32_t index, glm::vec3 rotation)
{
   targetRotation[index] = rotation;
   isChangingRotation[index] = true;
}

void WorldObject::setScale(uint32_t index, glm::vec3 scale)
{
   targetScale[index] = scale;
   isChangingScale[index] = true;
}

void WorldObject::setRotationSpeed(uint32_t index, float yaw, float pitch, float roll)
{
   rotationSpeed[index].x = yaw;
   rotationSpeed[index].y = pitch;
   rotationSpeed[index].z = roll;
}

void WorldObject::setMovingSpeed(uint32_t index, float movingSpeed)
{
   this->movingSpeed[index] = movingSpeed;
}

void WorldObject::setMovingDirection(uint32_t index, glm::vec3 movingDirection)
{
   this->movingDirection[index] = movingDirection;
}

void WorldObject::invalidateModelMatrix(uint32_t index)
{
   isModelMatrixInvalid[index] = true;
}

void WorldObject::createUniformBuffer()
{
   // model matrices (dynamic buffer)
   size_t uboAlignment = (size_t)vulkanDevice->deviceProperties.limits.minUniformBufferOffsetAlignment;
   dynamicAlignment = (sizeof(glm::mat4) / uboAlignment) * uboAlignment + ((sizeof(glm::mat4) % uboAlignment) > 0 ? uboAlignment : 0);

   size_t bufferSize = this->getNumberOfObjects() * dynamicAlignment;
   dynamicBufferSize = bufferSize;
   uboDataDynamic.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
   assert(uboDataDynamic.model);

   std::cout << "minUniformBufferOffsetAlignment = " << uboAlignment << std::endl;
   std::cout << "dynamicAlignment = " << dynamicAlignment << std::endl;

   vulkanDevice->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      &worldMatrixUBO.buffer,
      &worldMatrixUBO.memory);

   // TODO: make so map can be persistent. no need to map/unmap everytime we copy 
   // data in dynamic buffers.
   // in buffer class. have map() and unmap(). 
   // in destruction check if ma


   updateDynamicUniformBuffer();
}

void WorldObject::updateDynamicUniformBuffer()
{
   for(uint32_t i = 0; i < this->getNumberOfObjects(); i++)
   {
      glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (i * dynamicAlignment)));

      *modelMat = this->getModelMatrix(i);
   }

   void *data;
   vkMapMemory(vulkanDevice->device, worldMatrixUBO.memory, 0, dynamicBufferSize, 0, &data);

   memcpy(data, uboDataDynamic.model, dynamicBufferSize);

   VkMappedMemoryRange mappedMemoryRange{};
   mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

   mappedMemoryRange.memory = worldMatrixUBO.memory;
   mappedMemoryRange.size = dynamicBufferSize;
   vkFlushMappedMemoryRanges(vulkanDevice->device, 1, &mappedMemoryRange);

   vkUnmapMemory(vulkanDevice->device, worldMatrixUBO.memory);
}

void WorldObject::updateDescriptorSet()
{
   createUniformBuffer();

   VkDescriptorBufferInfo uboBufferInfo ={};
   uboBufferInfo.buffer = worldMatrixUBO.buffer;
   uboBufferInfo.offset = 0;
   uboBufferInfo.range  = VK_WHOLE_SIZE;

   std::array<VkWriteDescriptorSet, 1> descriptorWritesMatrix ={};
   descriptorWritesMatrix[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesMatrix[0].dstSet           = descriptorSet;
   descriptorWritesMatrix[0].dstBinding       = 1;
   descriptorWritesMatrix[0].dstArrayElement  = 0;
   descriptorWritesMatrix[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   descriptorWritesMatrix[0].descriptorCount  = 1;
   descriptorWritesMatrix[0].pBufferInfo      = &uboBufferInfo;
   descriptorWritesMatrix[0].pImageInfo       = nullptr;
   descriptorWritesMatrix[0].pTexelBufferView = nullptr;

   vkUpdateDescriptorSets(vulkanDevice->device, static_cast<uint32_t>(descriptorWritesMatrix.size()), descriptorWritesMatrix.data(), 0, nullptr);
}
