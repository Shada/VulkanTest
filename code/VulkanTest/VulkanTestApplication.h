#pragma once

#include "stdafx.h"
#include "VulkanShader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "WorldObject.h"
#include "VulkanDevice.hpp"

/// TODO: fix proper cleanup. currently lots of stuff that is not deleted correctly/at all
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
   } uniformBuffers;

   // TODO: move this into the mesh/object class .
   float animationTimer = 0.0f;

private:

   void mainLoop();

   void updateUniformBuffer();

   void drawFrame();

   // glfw stuff
   GLFWwindow* window;
   void initWindow();

   // vulkan stuff
   VulkanStuff vulkanStuff;

   VulkanDevice vulkanDevice;

   VkDebugReportCallbackEXT callback;
   
   // TODO: move texture to the mesh?
   Mesh *mesh;
   Texture *texture;
   WorldObject *worldObject;
   WorldObjectToMeshMapper *worldObjectToMeshMapper;

   // list of these, mesh needs to point at it. 
   VulkanShader vertShader;
   VulkanShader fragShader;

   VkDescriptorSetLayout descriptorSetLayoutMatrixBuffer;
   VkDescriptorSetLayout descriptorSetLayoutSampler;

   VkPipelineLayout pipelineLayout;

   VkRenderPass renderPass;

   VkPipeline graphicsPipeline;

   std::vector<VkFramebuffer> swapChainFrameBuffers;

   
   VkSemaphore imageAvailableSemaphore;
   VkSemaphore renderFinishedSemaphore;

   Camera camera;

   VkImage depthImage;
   VkDeviceMemory depthImageMemory;
   VkImageView depthImageView;

   VkDescriptorPool descriptorPool;

   VkDescriptorSet descriptorSetMatrixBuffer;
   std::vector<VkDescriptorSet> descriptorSetSampler;

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

   int createTextureImage(std::string filename);

   // These two are used for depth resource and swapchain. So I should probably make them helper functions, so I can use them from texture as well. 
   // or make them accessible from texture by changing the architecture.
   void createImage(uint32_t, uint32_t, VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage*, VkDeviceMemory*);

   VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags);

   void loadModel();

   void createUniformBuffer();

   void createDescriptorPool();

   void createDescriptorSet(int textureIndex);

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
   VkSwapchainKHR swapChain;
   std::vector<VkImage> swapChainImages;
   VkFormat swapChainImageFormat;
   VkExtent2D swapChainExtent;

   void createSwapChain();

   SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);

   VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);

   VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>);

   VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);

   //image views
   std::vector<VkImageView> swapChainImageViews;

   void createImageViews();


   // cleanup of vulkan stuff
   void cleanUp();
};