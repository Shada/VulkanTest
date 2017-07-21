#include "Texture.h"

Texture::Texture(const VulkanStuff *vulkanStuff)
{ 
   this->vulkanStuff = vulkanStuff;
}

Texture::~Texture()
{
   for(size_t i = 0; i < image.size(); i++)
   {
      vkFreeMemory(vulkanStuff->device, memory.at(i), nullptr);
      vkDestroyImage(vulkanStuff->device, image.at(i), nullptr);
      vkDestroyImageView(vulkanStuff->device, imageView.at(i), nullptr);
      vkDestroySampler(vulkanStuff->device, sampler.at(i), nullptr);
   }

}

int Texture::loadTexture(std::string filename)
{

   bool success = createImage(filename);

   if(success)
   {
      createImageView();
      createSampler();

      name.push_back(filename);

      return (int)name.size() - 1;
   }
   
   return -1;
}

bool Texture::createImage(std::string filename)
{
   int texWidth;
   int texHeight;
   int texChannels;

   stbi_uc* pixels = stbi_load(
      filename.c_str(),
      &texWidth,
      &texHeight,
      &texChannels,
      STBI_rgb_alpha);

   VkDeviceSize imageSize = texWidth * texHeight * 4;

   if(!pixels)
   {
      throw std::runtime_error("failed to load texture image!");
   }

   VkBuffer stagingBuffer;
   VkDeviceMemory stagingBufferMemory;

   // TODO: Create a helper h-file which have this function.
   createBuffer(
      imageSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingBufferMemory);

   void *data;
   vkMapMemory(vulkanStuff->device, stagingBufferMemory, 0, imageSize, 0, &data);
   memcpy(data, pixels, static_cast<size_t>(imageSize));
   vkUnmapMemory(vulkanStuff->device, stagingBufferMemory);

   stbi_image_free(pixels);

   VkImage tempTexture;
   VkDeviceMemory tempMemory;
   createVkImage(
      texWidth, texHeight,
      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &tempTexture, &tempMemory);

   transitionImageLayout(
      tempTexture,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_LAYOUT_PREINITIALIZED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

   copyBufferToImage(
      stagingBuffer,
      tempTexture,
      static_cast<uint32_t>(texWidth),
      static_cast<uint32_t>(texHeight));

   transitionImageLayout(
      tempTexture,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

   image.push_back(tempTexture);
   memory.push_back(tempMemory);

   vkDestroyBuffer(vulkanStuff->device, stagingBuffer, nullptr);
   vkFreeMemory(vulkanStuff->device, stagingBufferMemory, nullptr);

   return true;
}

void Texture::createImageView()
{
   VkImageView tempImageView;

   VkImageViewCreateInfo viewInfo ={};
   viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   viewInfo.image                           = image.back();
   viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
   viewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
   viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
   viewInfo.subresourceRange.baseMipLevel   = 0;
   viewInfo.subresourceRange.levelCount     = 1;
   viewInfo.subresourceRange.baseArrayLayer = 0;
   viewInfo.subresourceRange.layerCount     = 1;

   if(vkCreateImageView(vulkanStuff->device, &viewInfo, nullptr, &tempImageView) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create texture image view!");
   }

   imageView.push_back(tempImageView);
}


void Texture::createSampler()
{
   VkSamplerCreateInfo samplerInfo ={};
   samplerInfo.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   samplerInfo.magFilter = VK_FILTER_LINEAR;
   samplerInfo.minFilter = VK_FILTER_LINEAR;

   samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

   samplerInfo.anisotropyEnable = VK_TRUE;
   samplerInfo.maxAnisotropy    = 16;
   samplerInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

   samplerInfo.unnormalizedCoordinates = VK_FALSE;
   samplerInfo.compareEnable           = VK_FALSE;
   samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;

   samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   samplerInfo.mipLodBias = 0.0f;
   samplerInfo.minLod     = 0.0f;
   samplerInfo.maxLod     = 0.0f;

   VkSampler tempSampler;
   if(vkCreateSampler(vulkanStuff->device, &samplerInfo, nullptr, &tempSampler) != VK_SUCCESS)
   {
      throw std::runtime_error("Failed to create texture sampler!");
   }

   sampler.push_back(tempSampler);
}

// TODO: Maybe this should be a helper function.
void Texture::createVkImage(
   uint32_t width, uint32_t height,
   VkFormat format, VkImageTiling tiling,
   VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
   VkImage *image, VkDeviceMemory* imageMemory)
{
   VkImageCreateInfo imageInfo ={};
   imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.imageType     = VK_IMAGE_TYPE_2D;
   imageInfo.extent.width  = width;
   imageInfo.extent.height = height;
   imageInfo.extent.depth  = 1;
   imageInfo.mipLevels     = 1;
   imageInfo.arrayLayers   = 1;
   imageInfo.format        = format;
   imageInfo.tiling        = tiling;
   imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
   imageInfo.usage         = usage;
   imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
   imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

   if(vkCreateImage(vulkanStuff->device, &imageInfo, nullptr, image) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create image!");
   }

   VkMemoryRequirements memRequirements;
   vkGetImageMemoryRequirements(vulkanStuff->device, *image, &memRequirements);

   VkMemoryAllocateInfo allocInfo ={};
   allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize  = memRequirements.size;
   allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

   if(vkAllocateMemory(vulkanStuff->device, &allocInfo, nullptr, imageMemory) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate image memory!");
   }

   vkBindImageMemory(vulkanStuff->device, *image, *imageMemory, 0);
}






void Texture::createBuffer(
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