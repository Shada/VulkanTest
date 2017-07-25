#include "Mesh.h"

#include <tiny_obj_loader.h>
#include <unordered_map>

Mesh::Mesh(const VulkanStuff* vulkanStuff)
{
   this->vulkanStuff = vulkanStuff;
}

Mesh::~Mesh()
{
   for(auto &buffer : vertexBuffer)
   {
      vkDestroyBuffer(vulkanStuff->device, buffer, nullptr);
   }
   for(auto &memory : vertexBufferMemory)
   {
      vkFreeMemory(vulkanStuff->device, memory, nullptr);
   }
   for(auto &buffer : indexBuffer)
   {
      vkDestroyBuffer(vulkanStuff->device, buffer, nullptr);
   }
   for(auto &memory : indexBufferMemory)
   {
      vkFreeMemory(vulkanStuff->device, memory, nullptr);
   }
   vulkanStuff = nullptr;
}

// TODO: might want to try and make this look better.
void Mesh::loadMesh(const char* fileNames)
{
   VertexData tVertexData;

   tinyobj::attrib_t attrib;
   std::vector<tinyobj::shape_t> shapes;
   std::vector<tinyobj::material_t> materials;
   std::string err;
   std::unordered_map<Vertex, uint32_t>uniqueVertices ={};

   if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileNames))
   {
      throw std::runtime_error(err);
   }

   for(const auto& shape : shapes)
   {
      for(const auto& index : shape.mesh.indices)
      {
         Vertex vertex ={};

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

         if(uniqueVertices.count(vertex) == 0)
         {
            uniqueVertices[vertex] = static_cast<uint32_t>(tVertexData.vertices.size());
            tVertexData.vertices.push_back(vertex);
         }

         tVertexData.indices.push_back(static_cast<uint32_t>(uniqueVertices[vertex]));
      }
   }

   vertexData.push_back(tVertexData);

   createVertexBuffer();
   createIndexBuffer();

   modelName.push_back(fileNames);
}

Vertex Mesh::extractVertexFromAttrib(tinyobj::attrib_t attrib, const tinyobj::index_t index)
{
   Vertex vertex ={};

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

   return vertex;
}

void Mesh::createIndexBuffer()
{
   VkDeviceSize bufferSize = sizeof(vertexData.back().indices[0]) * vertexData.back().indices.size();

   VkBuffer stagingBuffer;
   VkDeviceMemory stagingMemory;

   createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingMemory);

   void* data;
   vkMapMemory(vulkanStuff->device, stagingMemory, 0, bufferSize, 0, &data);
   memcpy(data, vertexData.back().indices.data(), (size_t)bufferSize);
   vkUnmapMemory(vulkanStuff->device, stagingMemory);

   VkBuffer temp;
   VkDeviceMemory tempMem;
   createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &temp,
      &tempMem);

   copyBuffer(stagingBuffer, temp, bufferSize);

   indexBuffer.push_back(temp);
   indexBufferMemory.push_back(tempMem);

   vkDestroyBuffer(vulkanStuff->device, stagingBuffer, nullptr);
   vkFreeMemory(vulkanStuff->device, stagingMemory, nullptr);
}

void Mesh::createVertexBuffer()
{
   VkDeviceSize bufferSize = sizeof(vertexData.back().vertices[0])*vertexData.back().vertices.size();

   VkBuffer stagingBuffer;
   VkDeviceMemory stagingBufferMemory;

   createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingBufferMemory);

   void *data;
   vkMapMemory(vulkanStuff->device, stagingBufferMemory, 0, bufferSize, 0, &data);
   memcpy(data, vertexData.back().vertices.data(), (size_t)bufferSize);
   vkUnmapMemory(vulkanStuff->device, stagingBufferMemory);

   VkBuffer temp;
   VkDeviceMemory tempMem;
   
   createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &temp,
      &tempMem);


   copyBuffer(stagingBuffer, temp, bufferSize);

   vertexBuffer.push_back(temp);
   vertexBufferMemory.push_back(tempMem);

   vkDestroyBuffer(vulkanStuff->device, stagingBuffer, nullptr);
   vkFreeMemory(vulkanStuff->device, stagingBufferMemory, nullptr);
}

void Mesh::createBuffer(
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
