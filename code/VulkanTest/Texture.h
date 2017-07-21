#pragma once

// This class takes care of the entire texture creation process and holds the relevant texture data.
// the texture can be identified by name or an id. 
// the texture might or might not have a sampler. To start with, the sampler will just be null when a texture not have a sampler

// TODO: Make it possible to get image by name, or to search on image name or something like that.

#include "stdafx.h"
#include <math.h>
#include <stb_image.h>

class Texture
{
public:
   
   Texture(const VulkanStuff *vulkanStuff);
   ~Texture();

   int loadTexture(std::string filename); // returns the id of the texture

   size_t getNumImages()
   {
      return image.size();
   }

   VkImage getImage(int index)
   {
      return image[index];
   }

   VkImageView getImageView(int index)
   {
      return imageView[index];
   }

   VkSampler getSampler(int index)
   {
      return sampler[index];
   }

   std::string getImageName(int index)
   {
      return name[index];
   }

private:

   std::vector<VkImage> image;
   std::vector<VkDeviceMemory> memory;
   std::vector<VkSampler> sampler;
   std::vector<VkImageView> imageView;
   std::vector<glm::ivec2> imageSize;

   const VulkanStuff *vulkanStuff;

   std::vector<std::string> name; 

   bool createImage(std::string filename);
   void createVkImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage*, VkDeviceMemory*);
   void createSampler();
   void createImageView();

   // TODO: helper function
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

   // TODO: helper function ?
   void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
   {
      VkCommandBuffer commandBuffer = beginSingleTimeCommand();

      VkImageMemoryBarrier barrier ={};
      barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout                       = oldLayout;
      barrier.newLayout                       = newLayout;
      barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
      barrier.image                           = image;
      barrier.subresourceRange.baseMipLevel   = 0;
      barrier.subresourceRange.levelCount     = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount     = 1;

      if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
      {
         barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
         if(hasStencilComponent(format))
         {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
         }
      }
      else
      {
         barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      }

      if(oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED
         && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
      {
         barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
         barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      }
      else if(oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED &&
         newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
      {
         barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
         barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      }
      else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
         newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
      {
         barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      }
      else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
         newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
      {
         barrier.srcAccessMask = 0;
         barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      }
      else
      {
         throw std::invalid_argument("unsupported layout transition!");
      }

      vkCmdPipelineBarrier(
         commandBuffer,
         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
         0,
         0, nullptr,
         0, nullptr,
         1, &barrier);

      endSingleTimeCommand(commandBuffer);
   }


   // TODO: helper function
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

   // TODO: helper function
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

   // TODO: helper function ?
   bool hasStencilComponent(VkFormat format)
   {
      return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
   }


   // TODO: helper function ?
   void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
   {
      VkCommandBuffer commandBuffer = beginSingleTimeCommand();

      VkBufferImageCopy region ={};
      region.bufferOffset      = 0;
      region.bufferRowLength   = 0;
      region.bufferImageHeight = 0;

      region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel       = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount     = 1;

      region.imageOffset ={ 0, 0, 0 };
      region.imageExtent ={ width, height, 1 };

      vkCmdCopyBufferToImage(
         commandBuffer,
         buffer,
         image,
         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
         1,
         &region
      );

      endSingleTimeCommand(commandBuffer);
   }

   // TODO: helper function
   void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer*, VkDeviceMemory*);
};

