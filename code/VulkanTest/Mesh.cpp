#include "Mesh.h"

#include <unordered_map>

Mesh::Mesh(vks::VulkanDevice *vulkanDevice)
{
   this->vulkanDevice = vulkanDevice;

   texture = new Texture(vulkanDevice);
}

Mesh::~Mesh()
{
   for(auto &buffer : vertexBuffer)
   {
      vkDestroyBuffer(vulkanDevice->device, buffer, nullptr);
   }
   for(auto &memory : vertexBufferMemory)
   {
      vkFreeMemory(vulkanDevice->device, memory, nullptr);
   }
   for(auto &buffer : indexBuffer)
   {
      vkDestroyBuffer(vulkanDevice->device, buffer, nullptr);
   }
   for(auto &memory : indexBufferMemory)
   {
      vkFreeMemory(vulkanDevice->device, memory, nullptr);
   }

  // if(descriptorPool != VK_NULL_HANDLE)
  //    vkDestroyDescriptorPool(vulkanDevice->device, descriptorPool, nullptr);

   delete texture;
   texture = nullptr;
}


// TODO: might want to try and make this look better.
// TODO: handle loading of sub meshes. need to test how that works with the obj-loader. 
// one submesh per material would be reasonable. but how do i store it in a good way? 
// submesh points at a buffer (base mesh have the buffer) and startId/num indices. 
// all meshes have at least one submesh, even if the mesh only is one single mesh and is not divided.
// that makes it easy to optimize the code for each scenario. 


// TODO: I currently do not support models where a material is used several times in a model with other material(s) in between
void Mesh::loadMesh(const char* fileName)
{
   // Load TempMaterials.
   // create vector tempSubMeshes for each meterial
   

   // how to do when a material mot is used... shouldn't need to have a submesh for thatm material... should the material exist? 
   // doesn't harm to have an unused material I guess...
   // Ok for now, might want to figure out how best handle later. 
   // at least I should not create descriptorSet for these materials.. 
   // descriptor sets exists in the submesh, so unused materials should not be included.

   VertexData tVertexData;

   tinyobj::attrib_t attrib;
   std::vector<tinyobj::shape_t> shapes;
   std::vector<tinyobj::material_t> materials;
   std::string err;
   std::unordered_map<Vertex, uint32_t>uniqueVertices ={};
   
   if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName, "./models/"))
   {
      throw std::runtime_error(err);
   }

   uint32_t baseMaterialId = static_cast<uint32_t>(material.size());

   loadMaterials(materials);

   std::vector<SubMesh> tSubMeshes(materials.size());

   for(const auto& shape : shapes)
   {
      for(int i = 0; i < shape.mesh.indices.size(); i++)
      //for(const auto& index : shape.mesh.indices)
      {
         tinyobj::index_t index = shape.mesh.indices[i];

         Vertex vertex ={};
         
         extractVertexFromAttrib(vertex, attrib, index);

         int subMeshId = shape.mesh.material_ids[(int)std::floor(i / 3)];

         if(tSubMeshes[subMeshId].numberOfIndices == 0)
         {
            int materialId = baseMaterialId + subMeshId;
            tSubMeshes[subMeshId].materialId = materialId;
            tSubMeshes[subMeshId].startIndex = static_cast<uint32_t>(tVertexData.indices.size());
            tSubMeshes[subMeshId].meshId = static_cast<uint32_t>(modelName.size());
         }

         tSubMeshes[subMeshId].numberOfIndices++;

         if(uniqueVertices.count(vertex) == 0)
         {
            uniqueVertices[vertex] = static_cast<uint32_t>(tVertexData.vertices.size());
            tVertexData.vertices.push_back(vertex);
         }

         tVertexData.indices.push_back(static_cast<uint32_t>(uniqueVertices[vertex]));
      }
   }

   for(const auto& tSubMesh : tSubMeshes)
   {
      if(tSubMesh.numberOfIndices > 0)
      {
         subMeshMap[numberOfMeshes].push_back(tSubMesh);
      }
   }

   vertexData.push_back(tVertexData);

   createVertexBuffer();
   createIndexBuffer();

   modelName.push_back(fileName);

   numberOfMeshes++;
}

void Mesh::draw(int commandBufferIndex)
{
   //for loops here. for each mesh, for each instance
   // the objects need to be sorted by mesh to be optimized. this might need to be moved into a coupler class/struct to work best
  // vkCmdBindDescriptorSets(vulkanStuff->commandBuffers[commandBufferIndex], VkPipelineBindPoint(), 0, 0, 1, descriptor->getDescriptorSet(0), 0, nullptr);
   
}

inline void Mesh::loadMaterials(std::vector<tinyobj::material_t>& materials)
{
   for(const auto& material : materials)
   {
      Material tMaterial ={};

      if(material.diffuse_texname != "")
      {
         tMaterial.diffuseTextureId = texture->loadTexture("./textures/" + material.diffuse_texname);
      }
      if(material.specular_texname != "")
      {
         tMaterial.specularTextureId = texture->loadTexture("./textures/" + material.specular_texname);
      }
      if(material.bump_texname != "")
      {
         tMaterial.bumpTextureId = texture->loadTexture("./textures/" + material.bump_texname);
      }

      tMaterial.ambientColour = glm::vec3(material.ambient[0], material.ambient[1], material.ambient[2]);
      tMaterial.diffuseColour = glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
      tMaterial.specularColour = glm::vec3(material.specular[0], material.specular[1], material.specular[2]);

      this->material.push_back(tMaterial);
   }
}

inline void Mesh::extractVertexFromAttrib(Vertex& vertex, tinyobj::attrib_t& attrib, tinyobj::index_t& index)
{
   vertex.position =
   {
      attrib.vertices[3 * index.vertex_index + 0],
      attrib.vertices[3 * index.vertex_index + 1],
      attrib.vertices[3 * index.vertex_index + 2]
   };

   vertex.texCoord =
   {
      attrib.texcoords[2 * index.texcoord_index + 0],
      1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
   };

   vertex.colour ={ 1.0f,1.0f,1.0f };

}
void Mesh::createDescriptorSetLayout()
{
   // TODO: Add materia ubo 
   auto descriptorSetLayoutBinding = vulkan::initialisers::
      createDescriptorSetLayoutBinding(
         2,
         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         VK_SHADER_STAGE_FRAGMENT_BIT);

   auto layoutCreateInfo = vulkan::initialisers::
      createDescriptorSetLayoutCreateInfo(
         descriptorSetLayoutBinding);

   if(vkCreateDescriptorSetLayout(vulkanDevice->device, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create Mesh Material descriptor set layout!");
   }
}

void Mesh::createDescriptorPool()
{
   VkDescriptorPoolSize poolSize ={};
   poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   poolSize.descriptorCount = static_cast<uint32_t>(material.size());

   VkDescriptorPoolCreateInfo poolInfo ={};
   poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = 1;
   poolInfo.pPoolSizes    = &poolSize;
   poolInfo.maxSets       = static_cast<uint32_t>(material.size());

   if(vkCreateDescriptorPool(vulkanDevice->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create descriptor pool!");
   }
}

void Mesh::createDescriptorSet()
{
   for(auto& mat : material)
   {
      VkDescriptorSet tDescriptorSet;

      VkDescriptorSetAllocateInfo allocInfoSampler ={};
      allocInfoSampler.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfoSampler.descriptorPool     = descriptorPool;
      allocInfoSampler.descriptorSetCount = 1;
      allocInfoSampler.pSetLayouts        = &descriptorSetLayout;

      if(vkAllocateDescriptorSets(vulkanDevice->device, &allocInfoSampler, &tDescriptorSet) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to allocate MatrixBuffer descriptor set!");
      }

      VkDescriptorImageInfo samplerInfo={};
      samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      samplerInfo.imageView = texture->getImageView(mat.diffuseTextureId); // TODO: one for each material.
      samplerInfo.sampler = texture->getSampler(mat.diffuseTextureId);

      VkWriteDescriptorSet descriptorWritesSampler ={};
      descriptorWritesSampler.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWritesSampler.dstSet           = tDescriptorSet;
      descriptorWritesSampler.dstBinding       = 2;
      descriptorWritesSampler.dstArrayElement  = 0;
      descriptorWritesSampler.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWritesSampler.descriptorCount  = 1;
      descriptorWritesSampler.pBufferInfo      = nullptr;
      descriptorWritesSampler.pImageInfo       = &samplerInfo;
      descriptorWritesSampler.pTexelBufferView = nullptr;

      vkUpdateDescriptorSets(vulkanDevice->device, 1, &descriptorWritesSampler, 0, nullptr);

      descriptorSet.push_back(tDescriptorSet);
   }
}

// TODO: Merge to one method that takes argument about which type of buffer it is.
void Mesh::createIndexBuffer()
{
   VkDeviceSize bufferSize = sizeof(vertexData.back().indices[0]) * vertexData.back().indices.size();

   VkBuffer stagingBuffer;
   VkDeviceMemory stagingMemory;

   vulkanDevice->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingMemory);

   void* data;
   vkMapMemory(vulkanDevice->device, stagingMemory, 0, bufferSize, 0, &data);
   memcpy(data, vertexData.back().indices.data(), (size_t)bufferSize);
   vkUnmapMemory(vulkanDevice->device, stagingMemory);

   VkBuffer temp;
   VkDeviceMemory tempMem;
   vulkanDevice->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &temp,
      &tempMem);

   vulkanDevice->copyBuffer(stagingBuffer, temp, bufferSize);

   indexBuffer.push_back(temp);
   indexBufferMemory.push_back(tempMem);

   vkDestroyBuffer(vulkanDevice->device, stagingBuffer, nullptr);
   vkFreeMemory(vulkanDevice->device, stagingMemory, nullptr);
}

void Mesh::createVertexBuffer()
{
   VkDeviceSize bufferSize = sizeof(vertexData.back().vertices[0])*vertexData.back().vertices.size();

   VkBuffer stagingBuffer;
   VkDeviceMemory stagingBufferMemory;

   vulkanDevice->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingBufferMemory);

   void *data;
   vkMapMemory(vulkanDevice->device, stagingBufferMemory, 0, bufferSize, 0, &data);
   memcpy(data, vertexData.back().vertices.data(), (size_t)bufferSize);
   vkUnmapMemory(vulkanDevice->device, stagingBufferMemory);

   VkBuffer temp;
   VkDeviceMemory tempMem;
   
   vulkanDevice->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &temp,
      &tempMem);


   vulkanDevice->copyBuffer(stagingBuffer, temp, bufferSize);

   vertexBuffer.push_back(temp);
   vertexBufferMemory.push_back(tempMem);

   vkDestroyBuffer(vulkanDevice->device, stagingBuffer, nullptr);
   vkFreeMemory(vulkanDevice->device, stagingBufferMemory, nullptr);
}