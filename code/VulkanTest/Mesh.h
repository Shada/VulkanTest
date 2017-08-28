#pragma once

#include <vulkan\vulkan.hpp>
#include <tiny_obj_loader.h>
#include <string>
#include <map>

#include "VulkanHelpers.hpp"
#include "VulkanDevice.hpp"

#include "stdafx.h"
#include "Texture.h"


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
   Mesh(vks::VulkanDevice* vulkanDevice);
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

   vks::VulkanDevice* vulkanDevice;

   std::vector<VertexData> vertexData;

   std::vector<Material> material;

   // TODO: create struct/class that handles buffers. All types of buffers. 
   std::vector<VkBuffer> vertexBuffer;
   std::vector<VkDeviceMemory> vertexBufferMemory;
   std::vector<VkBuffer> indexBuffer;
   std::vector<VkDeviceMemory> indexBufferMemory;

   std::map<int, std::vector<SubMesh>> subMeshMap;

   uint32_t numberOfMeshes = 0;

   // TODO: might need a vector of these to make sure that I can expand with more descriptors if needed.
   // for example have a decriptorPool for up to ~100 meshes, and then create a new descriptorPool if it's needed.
   VkDescriptorPool descriptorPool;

   Texture *texture;

   std::vector<std::string> modelName;

   inline void loadMaterials(std::vector<tinyobj::material_t>& materials);

   inline void extractVertexFromAttrib(Vertex& vertex, tinyobj::attrib_t& attrib, tinyobj::index_t& index);
};