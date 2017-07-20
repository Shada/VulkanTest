#pragma once

#include "stdafx.h"
#include "VulkanShader.h"
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include "Camera.h"
#include "Mesh.h"

/// TODO: Cleanup. Remove using of VDeleter. It only complicates things for now. 
/// Not needed with proper cleanup.
class HelloTriangleApplication
{
private:
   // structs
   struct QueueFamilyIndices
   {
      int graphicsFamily = -1;
      int presentFamily = -1;
      bool isComplete()
      {
         return graphicsFamily >= 0 && presentFamily >= 0;
      }
   };

   struct SwapChainSupportDetails
   {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
   };

protected:
   static void onWindowResized(GLFWwindow*, int, int);

public:
   void run();

   void recreateSwapChain();

   struct UboDataDynamic
   {
      glm::mat4 *model = nullptr;
   } uboDataDynamic;

   struct
   {
      glm::mat4 view;
      glm::mat4 projection;
   } uboVS;

   struct
   {
      //TODO: Create Buffer class that takes care of buffers creation/memory handling etc.
      VkBuffer cameraBuffer;
      VkDeviceMemory cameraBufferMemory;
      VkBuffer dynamicBuffer;
      VkDeviceMemory dynamicBufferMemory;
   } uniformBuffers;

   // TODO: move this into the mesh/object class .
   float animationTimer = 0.0f;

   size_t dynamicAlignment;

private:

   void mainLoop();

   Camera::MatrixBufferObject* updateUniformBuffer();
   void updateDynamicUniformBuffer();

   void drawFrame();

   // glfw stuff
   GLFWwindow* window;
   void initWindow();

   // vulkan stuff
   VulkanStuff vulkanStuff;

   VDeleter<VkDebugReportCallbackEXT> callback{ vulkanStuff.instance, DestroyDebugReportCallbackEXT };
   
   Mesh *mesh;

   // list of these, mesh needs to point at it. 
   VulkanShader vertShader;
   VulkanShader fragShader;

   VDeleter<VkDescriptorSetLayout> descriptorSetLayout{ vulkanStuff.device, vkDestroyDescriptorSetLayout };

   VDeleter<VkPipelineLayout> pipelineLayout{ vulkanStuff.device, vkDestroyPipelineLayout };

   VDeleter<VkRenderPass> renderPass{ vulkanStuff.device, vkDestroyRenderPass };

   VDeleter<VkPipeline> graphicsPipeline{ vulkanStuff.device, vkDestroyPipeline };

   std::vector<VDeleter<VkFramebuffer>> swapChainFrameBuffers;

   
   VDeleter<VkSemaphore> imageAvailableSemaphore{ vulkanStuff.device, vkDestroySemaphore };
   VDeleter<VkSemaphore> renderFinishedSemaphore{ vulkanStuff.device, vkDestroySemaphore };

   std::vector<VkCommandBuffer> commandBuffers;


   Camera camera;

   // have to be careful so that we couple these together correctly.
   // safest bet is to have vector of a Texture struct containing one of each of these that represent a texture.
   // maybe even a texture class that a mesh or mesh/texture-coupler can point to with an ID. 
   std::vector<VkImage> textureImage;
   std::vector<VkDeviceMemory> textureImageMemory;
   std::vector<VkImageView> textureImageView;
   std::vector<VkSampler> textureSampler;

   VDeleter<VkImage> depthImage{ vulkanStuff.device, vkDestroyImage };
   VDeleter<VkDeviceMemory> depthImageMemory{ vulkanStuff.device, vkFreeMemory };
   VDeleter<VkImageView> depthImageView{ vulkanStuff.device, vkDestroyImageView };

   VDeleter<VkDescriptorPool> descriptorPool{ vulkanStuff.device, vkDestroyDescriptorPool };
   std::vector<VkDescriptorSet> descriptorSet; // TODO: This should be moved to a mesh class or something like that.

   VkViewport viewport ={};

   uint32_t findMemoryType(uint32_t typeFiter, VkMemoryPropertyFlags properties);

   void initVulkan();

   void createInstance();

   void setupDebugCallback();

   void createSurface();

   void pickPhysicalDevice();

   void createLogicalDevice();

   void createRenderPass();

   void createDescriptorSetLayout();

   void createGraphicsPipeline();

   void createFrameBuffers();

   void createCommandPool();

   void createDepthResources();

   void createTextureImage(std::string filename);

   void createImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage*, VkDeviceMemory*);

   void createTextureImageView();

   VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags);

   void createTextureSampler();

   void loadModel();

   void createUniformBuffer();

   void createDescriptorPool();

   void createDescriptorSet();

   void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer*, VkDeviceMemory*);

   void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);

   void createCommandBuffers();

   void createSemaphores();

   VkFormat findSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);

   VkFormat findDepthFormat();

   bool hasStencilComponent(VkFormat);

   VkCommandBuffer beginSingleTimeCommand();

   void endSingleTimeCommand(VkCommandBuffer);

   void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);

   void copyBufferToImage(VkBuffer, VkImage, uint32_t, uint32_t);

   QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);

   std::vector<const char*> getRequiredExtensions();

   bool checkValidationLayerSupport();

   bool isDeviceSuitable(VkPhysicalDevice);

   bool checkDeviceExtensionSupport(VkPhysicalDevice);

   //swap chain stuff
   VDeleter<VkSwapchainKHR> swapChain{ vulkanStuff.device, vkDestroySwapchainKHR };
   std::vector<VkImage> swapChainImages;
   VkFormat swapChainImageFormat;
   VkExtent2D swapChainExtent;

   void createSwapChain();

   SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);

   VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);

   VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>);

   VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);

   //image views
   std::vector<VDeleter<VkImageView>> swapChainImageViews;

   void createImageViews();
};