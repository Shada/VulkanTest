#include "VulkanTestApplication.h"
#include <set>
#include <algorithm>
#include <unordered_map>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
   VkDebugReportFlagsEXT flags,
   VkDebugReportObjectTypeEXT objType,
   uint64_t obj,
   size_t location,
   int32_t code,
   const char* layerPrefix,
   const char* msg,
   void* userData)
{
   std::cerr
      << "validation layer: "
      << msg
      << std::endl;

   return VK_FALSE;
}

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

void HelloTriangleApplication::run()
{
   initWindow();
   initVulkan();
   mainLoop();

   glfwDestroyWindow(window);
   glfwTerminate();

   cleanUp();

   delete mesh;
   mesh = nullptr;
}

void HelloTriangleApplication::initWindow()
{
   glfwInit();

   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

   window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

   camera.setWindowSize(WIDTH, HEIGHT);

   glfwSetWindowUserPointer(window, this);
   glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);
}

void HelloTriangleApplication::onWindowResized(GLFWwindow* window, int height, int width)
{
   if(width == 0 || height == 0)
   {
      return;
   }

   HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
   app->recreateSwapChain();

   app->camera.setWindowSize(width, height);
}

void HelloTriangleApplication::initVulkan()
{
   // this should be somewhere esle, like initApplication, initGame, or something like that..
   mesh = new Mesh(&vulkanStuff);

   texture = new Texture(&vulkanStuff);

   worldObjectToMeshMapper = new WorldObjectToMeshMapper();
   worldObject = new WorldObject(worldObjectToMeshMapper, &vulkanStuff);

   createInstance();
   setupDebugCallback();
   createSurface();
   pickPhysicalDevice();
   createLogicalDevice();
   createSwapChain();
   createImageViews();
   createRenderPass();
   createDescriptorSetLayout();
   createGraphicsPipeline();
   createCommandPool();
   createDepthResources();
   createFrameBuffers();
   int textureIndex = createTextureImage(TEXTURE_PATH_CHALET);
   loadModel();
   createUniformBuffer();
   createDescriptorPool();
   createDescriptorSet(textureIndex);
   textureIndex = createTextureImage(TEXTURE_PATH_STORMTROOPER);
   createDescriptorSet(textureIndex);


   int index = worldObject->addInstance(1, glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(.3f));
   worldObject->setRotationSpeed(index, 0.0f, 0.0f, 0.0f);

   createUniformBuffer();

   updateDescriptorSet(0);
   updateDescriptorSet(1);

   createCommandBuffers();
   createSemaphores();
}

void HelloTriangleApplication::pickPhysicalDevice()
{
   uint32_t deviceCount = 0;
   vkEnumeratePhysicalDevices(vulkanStuff.instance, &deviceCount, nullptr);

   if(deviceCount == 0)
   {
      throw std::runtime_error("failed to find GPUs with Vulkan support");
   }

   std::vector<VkPhysicalDevice> devices(deviceCount);
   vkEnumeratePhysicalDevices(vulkanStuff.instance, &deviceCount, devices.data());

   for(const auto& device : devices)
   {
      if(isDeviceSuitable(device))
      {
         vulkanStuff.physicalDevice = device;
         break;
      }
   }

   if(vulkanStuff.physicalDevice == VK_NULL_HANDLE)
   {
      throw std::runtime_error("failed to find a suitable GPU");
   }
}


HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device)
{
   QueueFamilyIndices indices;

   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

   std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

   VkBool32 presentSupport = false;
   int i = 0;

   for(const auto& queueFamily : queueFamilies)
   {
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkanStuff.surface, &presentSupport);

      if(queueFamily.queueCount > 0 && queueFamily.queueFlags&VK_QUEUE_GRAPHICS_BIT)
      {
         indices.graphicsFamily = i;
      }
      if(queueFamily.queueCount > 0 && presentSupport)
      {
         indices.presentFamily = i;
      }
      if(indices.isComplete())
      {
         break;
      }
      i++;
   }

   return indices;
}

void HelloTriangleApplication::createLogicalDevice()
{
   QueueFamilyIndices indices = findQueueFamilies(vulkanStuff.physicalDevice);

   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
   std::set<int> uniqueQueueFamilies =
   {
      indices.graphicsFamily,
      indices.presentFamily
   };

   float queuePriority = 1.0f;
   for(int queueFamily : uniqueQueueFamilies)
   {
      VkDeviceQueueCreateInfo queueCreateInfo ={};

      queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount       = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;

      queueCreateInfos.push_back(queueCreateInfo);
   }

   VkPhysicalDeviceFeatures deviceFeatures ={};
   deviceFeatures.samplerAnisotropy = VK_TRUE;

   VkDeviceCreateInfo createInfo ={};
   createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   createInfo.pQueueCreateInfos       = queueCreateInfos.data();
   createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
   createInfo.pEnabledFeatures        = &deviceFeatures;
   createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
   createInfo.ppEnabledExtensionNames = deviceExtensions.data();

   if(enableValidationLayers)
   {
      createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
   }
   else
   {
      createInfo.enabledLayerCount = 0;
   }

   if(vkCreateDevice(vulkanStuff.physicalDevice, &createInfo, nullptr, &vulkanStuff.device) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create logical device");
   }

   vkGetDeviceQueue(vulkanStuff.device, indices.graphicsFamily, 0, &vulkanStuff.graphicsQueue);
   vkGetDeviceQueue(vulkanStuff.device, indices.presentFamily, 0, &vulkanStuff.presentQueue);
}

void HelloTriangleApplication::createRenderPass()
{
   VkAttachmentDescription colorAttachment ={};
   colorAttachment.format         = swapChainImageFormat;
   colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
   colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference colorAttachmentRef ={};
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkAttachmentDescription depthAttachment ={};
   depthAttachment.format         = findDepthFormat();
   depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
   depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
   depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   VkAttachmentReference depthAttachmentRef ={};
   depthAttachmentRef.attachment = 1;
   depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass ={};
   subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount    = 1;
   subpass.pColorAttachments       = &colorAttachmentRef;
   subpass.pDepthStencilAttachment = &depthAttachmentRef;

   VkSubpassDependency dependency ={};
   dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass    = 0;
   dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

   std::array<VkAttachmentDescription, 2> attachments ={ colorAttachment, depthAttachment };
   VkRenderPassCreateInfo renderPassInfo ={};
   renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
   renderPassInfo.pAttachments    = attachments.data();
   renderPassInfo.subpassCount    = 1;
   renderPassInfo.pSubpasses      = &subpass;
   renderPassInfo.dependencyCount = 1;
   renderPassInfo.pDependencies   = &dependency;

   if(vkCreateRenderPass(vulkanStuff.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create render pass");
   }
}

void HelloTriangleApplication::createDescriptorSetLayout()
{
   worldObject->createDescriptorSetLayout();
   VkDescriptorSetLayoutBinding uboLayoutBindingMatrix =
      vulkan::initialisers::createDescriptorSetLayoutBinding(
         0,
         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         VK_SHADER_STAGE_VERTEX_BIT);

   VkDescriptorSetLayoutBinding dynamicUboLayoutBindingMatrix =
      vulkan::initialisers::createDescriptorSetLayoutBinding(
         1, 
         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 
         VK_SHADER_STAGE_VERTEX_BIT);

   std::vector<VkDescriptorSetLayoutBinding> bindingsMatrix =
   {
      uboLayoutBindingMatrix,
      dynamicUboLayoutBindingMatrix
   };


   VkDescriptorSetLayoutCreateInfo layoutInfoMatrix ={};
   layoutInfoMatrix.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfoMatrix.bindingCount = static_cast<uint32_t>(bindingsMatrix.size());
   layoutInfoMatrix.pBindings    = bindingsMatrix.data();

   if(vkCreateDescriptorSetLayout(vulkanStuff.device, &layoutInfoMatrix, nullptr, &descriptorSetLayoutMatrixBuffer) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create Matrix Buffer descriptor set layout!");
   }


   VkDescriptorSetLayoutBinding samplerLayoutBindingSampler =
      vulkan::initialisers::createDescriptorSetLayoutBinding(
         2,
         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         VK_SHADER_STAGE_FRAGMENT_BIT);

   std::vector<VkDescriptorSetLayoutBinding> bindingsSampler=
   {
      samplerLayoutBindingSampler
   };

   VkDescriptorSetLayoutCreateInfo layoutInfoSampler ={};
   layoutInfoSampler.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfoSampler.bindingCount = static_cast<uint32_t>(bindingsSampler.size());
   layoutInfoSampler.pBindings    = bindingsSampler.data();

   if(vkCreateDescriptorSetLayout(vulkanStuff.device, &layoutInfoSampler, nullptr, &descriptorSetLayoutSampler) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create Sampler descriptor set layout!");
   }
}

void HelloTriangleApplication::createGraphicsPipeline()
{
   vertShader.loadShader("shaders/vert.spv");
   fragShader.loadShader("shaders/frag.spv");

   vertShader.createShaderModule(vulkanStuff.device);
   fragShader.createShaderModule(vulkanStuff.device);

   auto vertShaderStageInfo = vertShader.createShaderStage(ShaderType::VERTEX);
   auto fragShaderStageInfo = fragShader.createShaderStage(ShaderType::FRAGMENT);

   VkPipelineShaderStageCreateInfo shaderStages[] =
   {
      vertShaderStageInfo,
      fragShaderStageInfo
   };

   auto bindingDescription    = Vertex::getBindingDescription();
   auto attributeDescriptions = Vertex::getAttributeDescriptions();

   VkPipelineVertexInputStateCreateInfo vertexInputInfo ={};
   vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount   = 1;
   vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
   vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
   vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

   VkPipelineInputAssemblyStateCreateInfo inputAssembly ={};
   inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssembly.primitiveRestartEnable = VK_FALSE;

   viewport.x        = 0.0f;
   viewport.y        = 0.0f;
   viewport.width    = static_cast<float>(swapChainExtent.width);
   viewport.height   = static_cast<float>(swapChainExtent.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor ={};
   scissor.offset ={ 0, 0 };
   scissor.extent = swapChainExtent;

   VkPipelineViewportStateCreateInfo viewportState ={};
   viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.pViewports    = &viewport;
   viewportState.scissorCount  = 1;
   viewportState.pScissors     = &scissor;

   VkPipelineRasterizationStateCreateInfo rasterizer ={};
   rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable        = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
   rasterizer.lineWidth               = 1.0f;
   rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
   rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
   rasterizer.depthBiasEnable         = VK_FALSE;
   rasterizer.depthBiasConstantFactor = 0.0f;
   rasterizer.depthBiasClamp          = 0.0f;
   rasterizer.depthBiasSlopeFactor    = 0.0f;

   VkPipelineMultisampleStateCreateInfo multisampling ={};
   multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable   = VK_FALSE;
   multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
   multisampling.minSampleShading      = 1.0f; // Optional
   multisampling.pSampleMask           = nullptr; // Optional
   multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
   multisampling.alphaToOneEnable      = VK_FALSE; // Optional

   VkPipelineColorBlendAttachmentState colorBlendAttachment ={};
   colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable         = VK_FALSE;
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo colorBlending ={};
   colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable     = VK_FALSE;
   colorBlending.logicOp           = VK_LOGIC_OP_COPY;
   colorBlending.attachmentCount   = 1;
   colorBlending.pAttachments      = &colorBlendAttachment;
   colorBlending.blendConstants[0] = 0.0f;
   colorBlending.blendConstants[1] = 0.0f;
   colorBlending.blendConstants[2] = 0.0f;
   colorBlending.blendConstants[3] = 0.0f;

   VkPipelineDepthStencilStateCreateInfo depthStencil={};
   depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depthStencil.depthTestEnable       = VK_TRUE;
   depthStencil.depthWriteEnable      = VK_TRUE;
   depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
   depthStencil.depthBoundsTestEnable = VK_FALSE;
   depthStencil.minDepthBounds        = 0.0f;
   depthStencil.maxDepthBounds        = 1.0f;
   depthStencil.stencilTestEnable     = VK_FALSE;
   depthStencil.front                 ={};
   depthStencil.back                  ={};

   VkDynamicState dynamicStates[] =
   {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_LINE_WIDTH
   };

   VkPipelineDynamicStateCreateInfo dynamicState ={};
   dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicState.dynamicStateCount = 2;
   dynamicState.pDynamicStates    = dynamicStates;

   std::vector<VkDescriptorSetLayout> descriptorSetLayouts =
   {
      descriptorSetLayoutMatrixBuffer,
      descriptorSetLayoutSampler
   };

   VkPipelineLayoutCreateInfo pipelineLayoutInfo ={};
   pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount         = 2;
   pipelineLayoutInfo.pSetLayouts            = descriptorSetLayouts.data();
   pipelineLayoutInfo.pushConstantRangeCount = 0;
   pipelineLayoutInfo.pPushConstantRanges    = nullptr;

   if(vkCreatePipelineLayout(vulkanStuff.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create pipeline layout!");
   }

   VkGraphicsPipelineCreateInfo pipelineInfo ={};
   pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.stageCount          = 2;
   pipelineInfo.pStages             = shaderStages;
   pipelineInfo.pVertexInputState   = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pViewportState      = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState   = &multisampling;
   pipelineInfo.pDepthStencilState  = &depthStencil;
   pipelineInfo.pColorBlendState    = &colorBlending;
   pipelineInfo.pDynamicState       = &dynamicState;
   pipelineInfo.layout              = pipelineLayout;
   pipelineInfo.renderPass          = renderPass;
   pipelineInfo.subpass             = 0;
   pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
   pipelineInfo.basePipelineIndex   = -1;

   if(vkCreateGraphicsPipelines(vulkanStuff.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create graphics pipeline");
   }
}

void HelloTriangleApplication::createFrameBuffers()
{
   swapChainFrameBuffers.resize(swapChainImageViews.size());

   for(size_t i = 0; i < swapChainImageViews.size(); i++)
   {
      std::array<VkImageView, 2> attachments =
      {
         swapChainImageViews[i],
         depthImageView
      };

      VkFramebufferCreateInfo framebufferInfo ={};
      framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass      = renderPass;
      framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      framebufferInfo.pAttachments    = attachments.data();
      framebufferInfo.width           = swapChainExtent.width;
      framebufferInfo.height          = swapChainExtent.height;
      framebufferInfo.layers          = 1;

      if(vkCreateFramebuffer(vulkanStuff.device, &framebufferInfo, nullptr, &swapChainFrameBuffers[i]) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to create framebuffer!");
      }
   }
}

void HelloTriangleApplication::createCommandPool()
{
   QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vulkanStuff.physicalDevice);

   VkCommandPoolCreateInfo poolInfo ={};
   poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
   poolInfo.flags            = 0;

   if(vkCreateCommandPool(vulkanStuff.device, &poolInfo, nullptr, &vulkanStuff.commandPool) != VK_SUCCESS)
   {
      throw std::runtime_error("faile to create command pool!");
   }
}

void HelloTriangleApplication::createDepthResources()
{
   VkFormat depthFormat = findDepthFormat();

   createImage(
      swapChainExtent.width, swapChainExtent.height,
      depthFormat,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &depthImage,
      &depthImageMemory
   );

   depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

   transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


/// TODO: couple together with the mesh. 
/// maybe move it into the mesh class? or separate class, but mesh have an instance?
int HelloTriangleApplication::createTextureImage(std::string filename)
{
   int textureIndex = texture->loadTexture(filename);
   if(textureIndex == -1)
   {
      throw std::runtime_error("failed to load texture image!");
   }
   return textureIndex;
}

void HelloTriangleApplication::createImage(
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

   if(vkCreateImage(vulkanStuff.device, &imageInfo, nullptr, image) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create image!");
   }

   VkMemoryRequirements memRequirements;
   vkGetImageMemoryRequirements(vulkanStuff.device, *image, &memRequirements);

   VkMemoryAllocateInfo allocInfo ={};
   allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize  = memRequirements.size;
   allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

   if(vkAllocateMemory(vulkanStuff.device, &allocInfo, nullptr, imageMemory) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate image memory!");
   }

   vkBindImageMemory(vulkanStuff.device, *image, *imageMemory, 0);
}

VkImageView HelloTriangleApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
   VkImageView imageView;

   VkImageViewCreateInfo viewInfo ={};
   viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
   viewInfo.image                           = image;
   viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
   viewInfo.format                          = format;
   viewInfo.subresourceRange.aspectMask     = aspectFlags;
   viewInfo.subresourceRange.baseMipLevel   = 0;
   viewInfo.subresourceRange.levelCount     = 1;
   viewInfo.subresourceRange.baseArrayLayer = 0;
   viewInfo.subresourceRange.layerCount     = 1;

   if(vkCreateImageView(vulkanStuff.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create texture image view!");
   }

   return imageView;
}

void HelloTriangleApplication::loadModel()
{
   mesh->loadMesh(MODEL_PATH_CUBE.c_str());
   mesh->loadMesh(MODEL_PATH_CUBE.c_str());

   worldObject->addInstance(0, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.3f));
   worldObject->addInstance(1, glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f), glm::vec3(0.3f));
   worldObject->addInstance(0, glm::vec3(1.f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(0.3f));

   worldObject->setRotationSpeed(1, 0.f, 0.f, 20.f);
   worldObject->setRotationSpeed(2, 0.f, 20.f, 20.f);
}

size_t dynamicBufferSize = 0;
void HelloTriangleApplication::createUniformBuffer()
{
   // camera buffer (view & projection matrices)
   size_t bufferSize = sizeof(camera.getCameraData());

   createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &uniformBuffers.cameraBuffer,
      &uniformBuffers.cameraBufferMemory);

   // model matrices (dynamic buffer)
   size_t uboAlignment = (size_t)vulkanStuff.deviceProperties.limits.minUniformBufferOffsetAlignment;
   dynamicAlignment = (sizeof(glm::mat4) / uboAlignment) * uboAlignment + ((sizeof(glm::mat4) % uboAlignment) > 0 ? uboAlignment : 0);

   bufferSize = worldObject->getNumberOfObjects() * dynamicAlignment;
   dynamicBufferSize = bufferSize;
   uboDataDynamic.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
   assert(uboDataDynamic.model);

   std::cout << "minUniformBufferOffsetAlignment = " << uboAlignment << std::endl;
   std::cout << "dynamicAlignment = " << dynamicAlignment << std::endl;

   createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      &uniformBuffers.dynamicBuffer,
      &uniformBuffers.dynamicBufferMemory);

   // TODO: make so map can be persistent. no need to map/unmap everytime we copy 
   // data in dynamic buffers.
   // in buffer class. have map() and unmap(). 
   // in destruction check if mapped data exists -> unpad then free memory.

   updateDynamicUniformBuffer();
}

void HelloTriangleApplication::createDescriptorPool()
{
   std::array<VkDescriptorPoolSize, 3> poolSizes ={};
   poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   poolSizes[0].descriptorCount = 200;
   poolSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   poolSizes[1].descriptorCount = 200;
   poolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   poolSizes[2].descriptorCount = 200;

   VkDescriptorPoolCreateInfo poolInfo ={};
   poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
   poolInfo.pPoolSizes    = poolSizes.data();
   poolInfo.maxSets       = 200;

   if(vkCreateDescriptorPool(vulkanStuff.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create descriptor pool!");
   }

   worldObject->createDescriptorPool();
}

// TODO: This should be moved in together with the Mesh/object. I think.. The only issue is the camera buffer.. But I can probably have a pointer in there somehow. Since I still need to do collision detection.
void HelloTriangleApplication::createDescriptorSet(int textureIndex)
{
   worldObject->createDescriptorSet();
   VkDescriptorSet tempDescriptor;
   //VkDescriptorSetLayout layouts[] ={ descriptorSetLayout };
   //
   //VkDescriptorSetAllocateInfo allocInfo ={};
   //allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   //allocInfo.descriptorPool     = descriptorPool;
   //allocInfo.descriptorSetCount = 1;
   //allocInfo.pSetLayouts        = layouts;
   //
   //if(vkAllocateDescriptorSets(vulkanStuff.device, &allocInfo, &tempDescriptor) != VK_SUCCESS)
   //{
   //   throw std::runtime_error("failed to allocate descriptor set!");
   //}
   //
   VkDescriptorBufferInfo uboBufferInfo ={};
   uboBufferInfo.buffer = uniformBuffers.cameraBuffer;
   uboBufferInfo.offset = 0;
   uboBufferInfo.range  = VK_WHOLE_SIZE;
   
   VkDescriptorBufferInfo dynamicBufferInfo ={};
   dynamicBufferInfo.buffer = uniformBuffers.dynamicBuffer;
   dynamicBufferInfo.offset = 0;
   dynamicBufferInfo.range  = VK_WHOLE_SIZE;
   
   VkDescriptorImageInfo imageSamplerInfo ={};
   imageSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   imageSamplerInfo.imageView   = texture->getImageView(textureIndex);
   imageSamplerInfo.sampler     = texture->getSampler(textureIndex);
   //
   //std::array<VkWriteDescriptorSet, 3> descriptorWrites ={};
   //descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   //descriptorWrites[0].dstSet           = tempDescriptor;
   //descriptorWrites[0].dstBinding       = 0;
   //descriptorWrites[0].dstArrayElement  = 0;
   //descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   //descriptorWrites[0].descriptorCount  = 1;
   //descriptorWrites[0].pBufferInfo      = &uboBufferInfo;
   //descriptorWrites[0].pImageInfo       = nullptr;
   //descriptorWrites[0].pTexelBufferView = nullptr;
   //
   //descriptorWrites[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   //descriptorWrites[1].dstSet           = tempDescriptor;
   //descriptorWrites[1].dstBinding       = 1;
   //descriptorWrites[1].dstArrayElement  = 0;
   //descriptorWrites[1].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   //descriptorWrites[1].descriptorCount  = 1;
   //descriptorWrites[1].pBufferInfo      = &dynamicBufferInfo;
   //descriptorWrites[1].pImageInfo       = nullptr;
   //descriptorWrites[1].pTexelBufferView = nullptr;
   //
   //descriptorWrites[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   //descriptorWrites[2].dstSet           = tempDescriptor;
   //descriptorWrites[2].dstBinding       = 2;
   //descriptorWrites[2].dstArrayElement  = 0;
   //descriptorWrites[2].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   //descriptorWrites[2].descriptorCount  = 1;
   //descriptorWrites[2].pBufferInfo      = nullptr;
   //descriptorWrites[2].pImageInfo       = &imageSamplerInfo;
   //descriptorWrites[2].pTexelBufferView = nullptr;
   //
   //vkUpdateDescriptorSets(vulkanStuff.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
   //
   //descriptorSet.push_back(tempDescriptor);




   VkDescriptorSetLayout layoutsMatrixBuffer[] ={ descriptorSetLayoutMatrixBuffer };

   VkDescriptorSetAllocateInfo allocInfoMatrixBuffer ={};
   allocInfoMatrixBuffer.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfoMatrixBuffer.descriptorPool     = descriptorPool;
   allocInfoMatrixBuffer.descriptorSetCount = 1;
   allocInfoMatrixBuffer.pSetLayouts        = layoutsMatrixBuffer;

   if(vkAllocateDescriptorSets(vulkanStuff.device, &allocInfoMatrixBuffer, &descriptorSetMatrixBuffer) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate MatrixBuffer descriptor set!");
   }

   std::array<VkWriteDescriptorSet, 2> descriptorWritesMatrixBuffer ={};
   descriptorWritesMatrixBuffer[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesMatrixBuffer[0].dstSet           = descriptorSetMatrixBuffer;
   descriptorWritesMatrixBuffer[0].dstBinding       = 0;
   descriptorWritesMatrixBuffer[0].dstArrayElement  = 0;
   descriptorWritesMatrixBuffer[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   descriptorWritesMatrixBuffer[0].descriptorCount  = 1;
   descriptorWritesMatrixBuffer[0].pBufferInfo      = &uboBufferInfo;
   descriptorWritesMatrixBuffer[0].pImageInfo       = nullptr;
   descriptorWritesMatrixBuffer[0].pTexelBufferView = nullptr;

   descriptorWritesMatrixBuffer[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesMatrixBuffer[1].dstSet           = descriptorSetMatrixBuffer;
   descriptorWritesMatrixBuffer[1].dstBinding       = 1;
   descriptorWritesMatrixBuffer[1].dstArrayElement  = 0;
   descriptorWritesMatrixBuffer[1].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   descriptorWritesMatrixBuffer[1].descriptorCount  = 1;
   descriptorWritesMatrixBuffer[1].pBufferInfo      = &dynamicBufferInfo;
   descriptorWritesMatrixBuffer[1].pImageInfo       = nullptr;
   descriptorWritesMatrixBuffer[1].pTexelBufferView = nullptr;

   vkUpdateDescriptorSets(vulkanStuff.device, static_cast<uint32_t>(descriptorWritesMatrixBuffer.size()), descriptorWritesMatrixBuffer.data(), 0, nullptr);



   
   VkDescriptorSetAllocateInfo allocInfoSampler ={};
   allocInfoSampler.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfoSampler.descriptorPool     = descriptorPool;
   allocInfoSampler.descriptorSetCount = 1;
   allocInfoSampler.pSetLayouts        = &descriptorSetLayoutSampler;

   if(vkAllocateDescriptorSets(vulkanStuff.device, &allocInfoSampler, &tempDescriptor) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate Sampler descriptor set!");
   }

   std::array<VkWriteDescriptorSet, 1> descriptorWritesSampler ={};

   descriptorWritesSampler[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesSampler[0].dstSet           = tempDescriptor;
   descriptorWritesSampler[0].dstBinding       = 2;
   descriptorWritesSampler[0].dstArrayElement  = 0;
   descriptorWritesSampler[0].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   descriptorWritesSampler[0].descriptorCount  = 1;
   descriptorWritesSampler[0].pBufferInfo      = nullptr;
   descriptorWritesSampler[0].pImageInfo       = &imageSamplerInfo;
   descriptorWritesSampler[0].pTexelBufferView = nullptr;

   vkUpdateDescriptorSets(vulkanStuff.device, static_cast<uint32_t>(descriptorWritesSampler.size()), descriptorWritesSampler.data(), 0, nullptr);

   descriptorSetSampler.push_back(tempDescriptor);

}
void HelloTriangleApplication::updateDescriptorSet(int index)
{

   VkDescriptorBufferInfo uboBufferInfo ={};
   uboBufferInfo.buffer = uniformBuffers.cameraBuffer;
   uboBufferInfo.offset = 0;
   uboBufferInfo.range  = VK_WHOLE_SIZE;

   VkDescriptorBufferInfo dynamicBufferInfo ={};
   dynamicBufferInfo.buffer = uniformBuffers.dynamicBuffer;
   dynamicBufferInfo.offset = 0;
   dynamicBufferInfo.range  = VK_WHOLE_SIZE;


   std::array<VkWriteDescriptorSet, 2> descriptorWritesMatrix ={};
   descriptorWritesMatrix[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesMatrix[0].dstSet           = descriptorSetMatrixBuffer;
   descriptorWritesMatrix[0].dstBinding       = 0;
   descriptorWritesMatrix[0].dstArrayElement  = 0;
   descriptorWritesMatrix[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   descriptorWritesMatrix[0].descriptorCount  = 1;
   descriptorWritesMatrix[0].pBufferInfo      = &uboBufferInfo;
   descriptorWritesMatrix[0].pImageInfo       = nullptr;
   descriptorWritesMatrix[0].pTexelBufferView = nullptr;

   descriptorWritesMatrix[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesMatrix[1].dstSet           = descriptorSetMatrixBuffer;
   descriptorWritesMatrix[1].dstBinding       = 1;
   descriptorWritesMatrix[1].dstArrayElement  = 0;
   descriptorWritesMatrix[1].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   descriptorWritesMatrix[1].descriptorCount  = 1;
   descriptorWritesMatrix[1].pBufferInfo      = &dynamicBufferInfo;
   descriptorWritesMatrix[1].pImageInfo       = nullptr;
   descriptorWritesMatrix[1].pTexelBufferView = nullptr;


   vkUpdateDescriptorSets(vulkanStuff.device, static_cast<uint32_t>(descriptorWritesMatrix.size()), descriptorWritesMatrix.data(), 0, nullptr);

}
void HelloTriangleApplication::createBuffer(
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

   if(vkCreateBuffer(vulkanStuff.device, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create vertex buffer!");
   }

   VkMemoryRequirements memoryRequirements;
   vkGetBufferMemoryRequirements(vulkanStuff.device, *buffer, &memoryRequirements);

   VkMemoryAllocateInfo allocateInfo ={};
   allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocateInfo.allocationSize  = memoryRequirements.size;
   allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

   if(vkAllocateMemory(vulkanStuff.device, &allocateInfo, nullptr, bufferMemory) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate vertex buffer memory!");
   }

   vkBindBufferMemory(vulkanStuff.device, *buffer, *bufferMemory, 0);
}

void HelloTriangleApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
   VkCommandBuffer commandBuffer = beginSingleTimeCommand();

   VkBufferCopy bufferCopy ={};
   bufferCopy.srcOffset = 0;
   bufferCopy.dstOffset = 0;
   bufferCopy.size      = size;

   vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

   endSingleTimeCommand(commandBuffer);
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFiter, VkMemoryPropertyFlags properties)
{
   VkPhysicalDeviceMemoryProperties memoryProperties;
   vkGetPhysicalDeviceMemoryProperties(vulkanStuff.physicalDevice, &memoryProperties);

   for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
   {
      if(typeFiter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags&properties) == properties)
      {
         return i;
      }
   }

   throw std::runtime_error("failed to find suitable memory type!");
}

void HelloTriangleApplication::createCommandBuffers()
{
   vulkanStuff.commandBuffers.resize(swapChainFrameBuffers.size());

   VkCommandBufferAllocateInfo allocInfo ={};
   allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool        = vulkanStuff.commandPool;
   allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = (uint32_t)vulkanStuff.commandBuffers.size();

   if(vkAllocateCommandBuffers(vulkanStuff.device, &allocInfo, vulkanStuff.commandBuffers.data()) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create command buffers!");
   }

   VkCommandBufferBeginInfo beginInfo ={};
   beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
   beginInfo.pInheritanceInfo = nullptr;

   std::array<VkClearValue, 2> clearValues ={};
   clearValues[0].color ={ 0.0f, 0.0f, 0.0f, 0.0f };
   clearValues[1].depthStencil ={ 1.0f, 0 };

   VkRenderPassBeginInfo renderPassBeginInfo ={};
   renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassBeginInfo.renderPass        = renderPass;
   renderPassBeginInfo.renderArea.offset ={ 0,0 };
   renderPassBeginInfo.renderArea.extent = swapChainExtent;
   renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
   renderPassBeginInfo.pClearValues      = clearValues.data();

   for(size_t i = 0; i < vulkanStuff.commandBuffers.size(); i++)
   {
      renderPassBeginInfo.framebuffer = swapChainFrameBuffers[i];

      if(vkBeginCommandBuffer(vulkanStuff.commandBuffers[i], &beginInfo) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to begin command buffer recording!");
      }

      vkCmdBeginRenderPass(vulkanStuff.commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdSetViewport(vulkanStuff.commandBuffers[i], 0, 1, &viewport);

      vkCmdBindPipeline(vulkanStuff.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

      VkDeviceSize offsets[] ={ 0 };

      for(uint32_t j = 0; j < worldObject->getNumberOfObjects(); j++)
      {
         uint32_t meshId = worldObject->getMeshId(j);
         VkBuffer vertexBuffers[] ={ mesh->getVertexBuffer(meshId) };
         vkCmdBindVertexBuffers(vulkanStuff.commandBuffers[i], 0, 1, vertexBuffers, offsets);

         vkCmdBindIndexBuffer(vulkanStuff.commandBuffers[i], mesh->getIndexBuffer(meshId), 0, VK_INDEX_TYPE_UINT32);

         uint32_t dynamicOffset = j * static_cast<uint32_t>(dynamicAlignment);

         vkCmdBindDescriptorSets(vulkanStuff.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetMatrixBuffer, 1, &dynamicOffset);
         vkCmdBindDescriptorSets(vulkanStuff.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSetSampler[meshId], 0, nullptr);
         
         vkCmdDrawIndexed(vulkanStuff.commandBuffers[i], mesh->getNumIndices(meshId), 1, 0, 0, 0);
      }


      // new draw procedure for when descriptors have been fixed

      //for(uint32_t j=0; j < mesh->getNumberOfMeshes(); j++)
      //{
      //   // bind vertex and index buffers here
      //   for(uint32_t k=0; k < worldObjectToMeshMapper->getWorldObjectIdsForMesh(j).size(); k++)
      //   {
      //      // calculate dynamic offset here or set uniform buffer here, by binding a descriptor from the descriptorset
      //      for(uint32_t l = 0; l < mesh->getSubMeshesForMesh(j).size(); l++)
      //      {
      //         // bind descriptor set here or bind the part with textures 
      //
      //         // draw indexed using the submesh data to get start index and num indices
      //      }
      //   }
      //}

      vkCmdEndRenderPass(vulkanStuff.commandBuffers[i]);

      if(vkEndCommandBuffer(vulkanStuff.commandBuffers[i]) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to record command buffer!");
      }
   }
}

void HelloTriangleApplication::createSemaphores()
{
   VkSemaphoreCreateInfo semaphoreInfo ={};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   if(vkCreateSemaphore(vulkanStuff.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(vulkanStuff.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create semaphores!");
   }
}

VkFormat HelloTriangleApplication::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
   for(VkFormat format : candidates)
   {
      VkFormatProperties properties;
      vkGetPhysicalDeviceFormatProperties(vulkanStuff.physicalDevice, format, &properties);

      if(tiling == VK_IMAGE_TILING_LINEAR &&
         (properties.linearTilingFeatures & features) == features)
      {
         return format;
      }
      else if(tiling == VK_IMAGE_TILING_OPTIMAL &&
         (properties.optimalTilingFeatures & features) == features)
      {
         return format;
      }
   }

   throw std::runtime_error("failed to find supported format!");
}

VkFormat HelloTriangleApplication::findDepthFormat()
{
   return findSupportedFormat(
   { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
   );
}

bool HelloTriangleApplication::hasStencilComponent(VkFormat format)
{
   return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
      format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkCommandBuffer HelloTriangleApplication::beginSingleTimeCommand()
{
   VkCommandBufferAllocateInfo allocInfo ={};
   allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandPool        = vulkanStuff.commandPool;
   allocInfo.commandBufferCount = 1;

   VkCommandBuffer commandBuffer;
   vkAllocateCommandBuffers(vulkanStuff.device, &allocInfo, &commandBuffer);

   VkCommandBufferBeginInfo beginInfo ={};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   vkBeginCommandBuffer(commandBuffer, &beginInfo);

   return commandBuffer;
}

void HelloTriangleApplication::endSingleTimeCommand(VkCommandBuffer commandBuffer)
{
   vkEndCommandBuffer(commandBuffer);

   VkSubmitInfo submitInfo ={};
   submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers    = &commandBuffer;

   vkQueueSubmit(vulkanStuff.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

   vkQueueWaitIdle(vulkanStuff.graphicsQueue);

   vkFreeCommandBuffers(vulkanStuff.device, vulkanStuff.commandPool, 1, &commandBuffer);
}

void HelloTriangleApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
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

void HelloTriangleApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
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

void HelloTriangleApplication::recreateSwapChain()
{
   vkDeviceWaitIdle(vulkanStuff.device);

   createSwapChain();
   createImageViews();
   createRenderPass();
   createGraphicsPipeline();
   createDepthResources();
   createFrameBuffers();
   createCommandBuffers();
}

void HelloTriangleApplication::createImageViews()
{
   swapChainImageViews.resize(swapChainImages.size());

   for(uint32_t i = 0; i < swapChainImages.size(); i++)
   {
      swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
   }
}

void HelloTriangleApplication::cleanUp()
{
   vkFreeMemory(vulkanStuff.device, uniformBuffers.cameraBufferMemory, nullptr);
   vkDestroyBuffer(vulkanStuff.device, uniformBuffers.cameraBuffer, nullptr);

   vkFreeMemory(vulkanStuff.device, uniformBuffers.dynamicBufferMemory, nullptr);
   vkDestroyBuffer(vulkanStuff.device, uniformBuffers.dynamicBuffer, nullptr);

   delete texture;
   texture = nullptr;
}

void HelloTriangleApplication::createSwapChain()
{
   SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulkanStuff.physicalDevice);

   VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
   VkPresentModeKHR presentMode     = chooseSwapPresentMode(swapChainSupport.presentModes);
   VkExtent2D extent                = chooseSwapExtent(swapChainSupport.capabilities);

   uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
   if(swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount)
   {
      imageCount = swapChainSupport.capabilities.maxImageCount;
   }

   VkSwapchainCreateInfoKHR createInfo={};
   createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   createInfo.surface          = vulkanStuff.surface;
   createInfo.minImageCount    = imageCount;
   createInfo.imageFormat      = surfaceFormat.format;
   createInfo.imageColorSpace  = surfaceFormat.colorSpace;
   createInfo.imageExtent      = extent;
   createInfo.imageArrayLayers = 1;
   createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

   QueueFamilyIndices indices = findQueueFamilies(vulkanStuff.physicalDevice);

   uint32_t queueFamilyIndicies[] ={ (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

   if(indices.graphicsFamily != indices.presentFamily)
   {
      createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices   = queueFamilyIndicies;
   }
   else
   {
      createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices   = nullptr;
   }

   createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
   createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   createInfo.presentMode    = presentMode;
   createInfo.clipped        = VK_TRUE;
   createInfo.oldSwapchain   = VK_NULL_HANDLE;

   if(vkCreateSwapchainKHR(vulkanStuff.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create swap chain!");
   }

   vkGetSwapchainImagesKHR(vulkanStuff.device, swapChain, &imageCount, nullptr);

   swapChainImages.resize(imageCount);

   vkGetSwapchainImagesKHR(vulkanStuff.device, swapChain, &imageCount, swapChainImages.data());

   swapChainImageFormat = surfaceFormat.format;
   swapChainExtent      = extent;
}

bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device)
{
   vkGetPhysicalDeviceProperties(device, &vulkanStuff.deviceProperties);
   vkGetPhysicalDeviceFeatures(device, &vulkanStuff.deviceFeatures);

   QueueFamilyIndices indices = findQueueFamilies(device);

   bool extensionsSupported = checkDeviceExtensionSupport(device);

   bool swapChainAdequate = false;
   if(extensionsSupported)
   {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);

      swapChainAdequate =
         !swapChainSupport.formats.empty() &&
         !swapChainSupport.presentModes.empty();
   }

   return
      indices.isComplete() &&
      vulkanStuff.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
      vulkanStuff.deviceFeatures.geometryShader &&
      vulkanStuff.deviceFeatures.samplerAnisotropy &&
      extensionsSupported &&
      swapChainAdequate;
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

   std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

   for(const auto& extension : availableExtensions)
   {
      requiredExtensions.erase(extension.extensionName);
   }

   return requiredExtensions.empty();
}

HelloTriangleApplication::SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device)
{
   SwapChainSupportDetails details;

   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkanStuff.surface, &details.capabilities);

   uint32_t formatCount;
   vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanStuff.surface, &formatCount, nullptr);

   if(formatCount != 0)
   {
      details.formats.resize(formatCount);

      vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanStuff.surface, &formatCount, details.formats.data());
   }

   uint32_t presentModeCount;
   vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanStuff.surface, &presentModeCount, nullptr);

   if(presentModeCount != 0)
   {
      details.presentModes.resize(presentModeCount);

      vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanStuff.surface, &presentModeCount, details.presentModes.data());
   }

   return details;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
   if(availableFormats.size() == 1 &&
      availableFormats[0].format == VK_FORMAT_UNDEFINED)
   {
      return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
   }

   for(const auto& availableFormat : availableFormats)
   {
      if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM&&availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
         return availableFormat;
      }
   }

   return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
   VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

   for(const auto& availablePresentMode : availablePresentModes)
   {
      if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
         return availablePresentMode;
      }
      else if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
         bestMode = availablePresentMode;
      }
   }

   return bestMode;
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
   if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
   {
      return capabilities.currentExtent;
   }
   else
   {
      int width, height;
      glfwGetWindowSize(window, &width, &height);

      VkExtent2D actualExtent ={ (uint32_t)width, (uint32_t)height };

      actualExtent.width  = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

      return actualExtent;
   }
}

void HelloTriangleApplication::mainLoop()
{
   int frame = 0;
   long long timediff = 0;
   long long dt = 0;
   while(!glfwWindowShouldClose(window))
   {
      auto t1 = std::chrono::high_resolution_clock::now();

      glfwPollEvents();

      worldObject->update(float((double)dt / 1e9f));

      updateUniformBuffer();
      updateDynamicUniformBuffer();

      drawFrame();

      auto t2 = std::chrono::high_resolution_clock::now();

      dt = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
      timediff += dt;
      frame++;

      if(timediff > 1000000000)
      {
         std::cout << "FPS: " << frame << std::endl;
         timediff = 0;
         frame = 0;
      }
   }

   vkDeviceWaitIdle(vulkanStuff.device);
}

void HelloTriangleApplication::updateUniformBuffer()
{
   // TODO: change this stuff, it's weird

   camera.rotate(); // TODO: Should only bew camera->update(), and not called in this method.

   Camera::MatrixBufferObject *mbo = &camera.getCameraData();

   uboVS.projection = mbo->projectionMatrix;
   uboVS.view = mbo->viewMatrix;

   void *data;
   vkMapMemory(vulkanStuff.device, uniformBuffers.cameraBufferMemory, 0, sizeof(uboVS), 0, &data);
   memcpy(data, &uboVS, sizeof(uboVS));
   vkUnmapMemory(vulkanStuff.device, uniformBuffers.cameraBufferMemory);

}

void HelloTriangleApplication::updateDynamicUniformBuffer()
{
   for(uint32_t i = 0; i < worldObject->getNumberOfObjects(); i++)
   {
      glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (i * dynamicAlignment)));

      *modelMat = worldObject->getModelMatrix(i);
   }

   void *data;
   vkMapMemory(vulkanStuff.device, uniformBuffers.dynamicBufferMemory, 0, dynamicBufferSize, 0, &data);

   memcpy(data, uboDataDynamic.model, dynamicBufferSize);

   VkMappedMemoryRange mappedMemoryRange{};
   mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

   mappedMemoryRange.memory = uniformBuffers.dynamicBufferMemory;
   mappedMemoryRange.size = dynamicBufferSize;
   vkFlushMappedMemoryRanges(vulkanStuff.device, 1, &mappedMemoryRange);

   vkUnmapMemory(vulkanStuff.device, uniformBuffers.dynamicBufferMemory);
}

void HelloTriangleApplication::drawFrame()
{
   uint32_t imageIndex;
   VkResult result = vkAcquireNextImageKHR(
      vulkanStuff.device,
      swapChain,
      std::numeric_limits<uint64_t>::max(),
      imageAvailableSemaphore,
      VK_NULL_HANDLE,
      &imageIndex);

   if(result == VK_ERROR_OUT_OF_DATE_KHR)
   {
      recreateSwapChain();
      return;
   }
   else if(result != VK_SUCCESS &&
      result != VK_SUBOPTIMAL_KHR)
   {
      throw std::runtime_error("failed to acquire swap chain image!");
   }

   VkSubmitInfo submitInfo ={};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   VkSemaphore waitSemaphores[] ={ imageAvailableSemaphore };

   VkPipelineStageFlags waitStages[] ={ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores    = waitSemaphores;
   submitInfo.pWaitDstStageMask  = waitStages;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers    = &vulkanStuff.commandBuffers[imageIndex];

   VkSemaphore signalSemaphores[] ={ renderFinishedSemaphore };
   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores    = signalSemaphores;

   if(vkQueueSubmit(vulkanStuff.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to submit draw command buffer");
   }

   VkPresentInfoKHR presentInfo ={};
   presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   presentInfo.waitSemaphoreCount = 1;
   presentInfo.pWaitSemaphores    = signalSemaphores;

   VkSwapchainKHR swapChains[] ={ swapChain };
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains    = swapChains;
   presentInfo.pImageIndices  = &imageIndex;
   presentInfo.pResults       = nullptr;

   result = vkQueuePresentKHR(vulkanStuff.presentQueue, &presentInfo);

   if(result == VK_ERROR_OUT_OF_DATE_KHR ||
      result == VK_SUBOPTIMAL_KHR)
   {
      recreateSwapChain();
   }
   else if(result != VK_SUCCESS)
   {
      throw std::runtime_error("failed to present swap chain image!");
   }
}

void HelloTriangleApplication::createInstance()
{
   if(enableValidationLayers && !checkValidationLayerSupport())
   {
      throw std::runtime_error("validation layers requested, but not available!");
   }

   VkApplicationInfo appInfo	={};
   appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName   = "Hello Triangle";
   appInfo.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName        = "No Engine";
   appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion         = VK_API_VERSION_1_0;

   VkInstanceCreateInfo createInfo ={};
   createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;

   auto extensions = getRequiredExtensions();

   createInfo.enabledExtensionCount	  = static_cast<uint32_t>(extensions.size());
   createInfo.ppEnabledExtensionNames = extensions.data();

   if(enableValidationLayers)
   {
      createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
   }
   else
   {
      createInfo.enabledLayerCount = 0;
   }

   if(vkCreateInstance(&createInfo, nullptr, &vulkanStuff.instance) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create instance!");
   }
}

void HelloTriangleApplication::setupDebugCallback()
{
   if(!enableValidationLayers) return;

   VkDebugReportCallbackCreateInfoEXT createInfo ={};
   createInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
   createInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
   createInfo.pfnCallback = debugCallback;

   if(CreateDebugReportCallbackEXT(vulkanStuff.instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to set up debug callback!");
   }
}

void HelloTriangleApplication::createSurface()
{
   if(glfwCreateWindowSurface(vulkanStuff.instance, window, nullptr, &vulkanStuff.surface) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create window surface!");
   }
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
{
   std::vector<const char*> extensions;

   unsigned int glfwExtensionCount = 0;

   const char** glfwExtensions;

   glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

   for(unsigned int i = 0; i < glfwExtensionCount; i++)
   {
      extensions.push_back(glfwExtensions[i]);
   }

   if(enableValidationLayers)
   {
      extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
   }

   return extensions;
}

bool HelloTriangleApplication::checkValidationLayerSupport()
{
   uint32_t layerCount;
   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

   std::vector<VkLayerProperties> availableLayers(layerCount);
   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

   for(const char* layerName : validationLayers)
   {
      bool layerFound = false;

      for(const auto& layerProperties : availableLayers)
      {
         if(strcmp(layerName, layerProperties.layerName) == 0)
         {
            layerFound = true;
            break;
         }
      }

      if(!layerFound)
      {
         return false;
      }
   }
   return true;

}