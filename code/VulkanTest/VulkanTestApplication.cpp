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

   delete worldObject;
   worldObject = nullptr;

   delete worldObjectToMeshMapper;
   worldObjectToMeshMapper = nullptr;
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
   // TODO: restructure this, maybe I might want mesh / object loading somewhere by itselves..
   // this should be somewhere esle, like initApplication, initGame, or something like that..
   mesh = new Mesh(&vulkanDevice);

   worldObjectToMeshMapper = new WorldObjectToMeshMapper();
   worldObject = new WorldObject(worldObjectToMeshMapper, &vulkanDevice);

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
   loadModel();
   createUniformBuffer();
   createDescriptorPool();
   worldObject->createDescriptorPool();
   createDescriptorSet();
   worldObject->createDescriptorSet();

   int index = worldObject->addInstance(1, glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(.3f));
   worldObject->setRotationSpeed(index, 0.0f, 0.0f, 0.0f);

   // TODO: automatically call this from add instance, using a boolean: if true -> do
   worldObject->updateDescriptorSet();

   createCommandBuffers();
   createSemaphores();
}

// TODO : Save all available devices in some sort of list, so that the user could choose device in options if necessary
// TODO: Move the selection of physical tdevice to vulkandevice struct??

void HelloTriangleApplication::pickPhysicalDevice()
{
   uint32_t deviceCount = 0;
   vkEnumeratePhysicalDevices(vulkanDevice.instance, &deviceCount, nullptr);

   if(deviceCount == 0)
   {
      throw std::runtime_error("failed to find GPUs with Vulkan support");
   }

   std::vector<VkPhysicalDevice> devices(deviceCount);
   vkEnumeratePhysicalDevices(vulkanDevice.instance, &deviceCount, devices.data());

   bool founddevice = false;
   for(const auto& device : devices)
   {
      vulkanDevice.physicalDevice = device;
      if(isDeviceSuitable(device))
      {
         founddevice = true;
         break;
      }
   }

   if(!founddevice || vulkanDevice.physicalDevice == VK_NULL_HANDLE)
   {
      throw std::runtime_error("failed to find a suitable GPU");
   }
}

void HelloTriangleApplication::createLogicalDevice()
{
   vulkanDevice.createLogicalDevice();
   /*
   QueueFamilyIndices indices = findQueueFamilies(vulkanDevice.physicalDevice);

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

   if(vkCreateDevice(vulkanDevice.physicalDevice, &createInfo, nullptr, &vulkanDevice.device) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create logical device");
   }

   vkGetDeviceQueue(vulkanDevice.device, indices.graphicsFamily, 0, &vulkanStuff.graphicsQueue);
   vkGetDeviceQueue(vulkanDevice.device, indices.presentFamily, 0, &vulkanStuff.presentQueue);
   */
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

   if(vkCreateRenderPass(vulkanDevice.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
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

   std::vector<VkDescriptorSetLayoutBinding> bindingsMatrix =
   {
      uboLayoutBindingMatrix
   };
   
   VkDescriptorSetLayoutCreateInfo layoutInfoMatrix ={};
   layoutInfoMatrix.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   layoutInfoMatrix.bindingCount = static_cast<uint32_t>(bindingsMatrix.size());
   layoutInfoMatrix.pBindings    = bindingsMatrix.data();

   if(vkCreateDescriptorSetLayout(vulkanDevice.device, &layoutInfoMatrix, nullptr, &descriptorSetLayoutMatrixBuffer) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create Matrix Buffer descriptor set layout!");
   }

   mesh->createDescriptorSetLayout();
}

void HelloTriangleApplication::createGraphicsPipeline()
{
   vertShader.loadShader("shaders/vert.spv");
   fragShader.loadShader("shaders/frag.spv");

   vertShader.createShaderModule(vulkanDevice.device);
   fragShader.createShaderModule(vulkanDevice.device);

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
      worldObject->getDescriptorSetLayout(),
      mesh->getDescriptorSetLayout()
   };

   VkPipelineLayoutCreateInfo pipelineLayoutInfo ={};
   pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount         = static_cast<uint32_t>(descriptorSetLayouts.size());
   pipelineLayoutInfo.pSetLayouts            = descriptorSetLayouts.data();
   pipelineLayoutInfo.pushConstantRangeCount = 0;
   pipelineLayoutInfo.pPushConstantRanges    = nullptr;

   if(vkCreatePipelineLayout(vulkanDevice.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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

   if(vkCreateGraphicsPipelines(vulkanDevice.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
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

      if(vkCreateFramebuffer(vulkanDevice.device, &framebufferInfo, nullptr, &swapChainFrameBuffers[i]) != VK_SUCCESS)
      {
         throw std::runtime_error("failed to create framebuffer!");
      }
   }
}

void HelloTriangleApplication::createCommandPool()
{
   vks::QueueFamilyIndices queueFamilyIndices = vulkanDevice.findQueueFamilies();

   VkCommandPoolCreateInfo poolInfo ={};
   poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
   poolInfo.flags            = 0;

   if(vkCreateCommandPool(vulkanDevice.device, &poolInfo, nullptr, &vulkanDevice.commandPool) != VK_SUCCESS)
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

   if(vkCreateImage(vulkanDevice.device, &imageInfo, nullptr, image) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create image!");
   }

   VkMemoryRequirements memRequirements;
   vkGetImageMemoryRequirements(vulkanDevice.device, *image, &memRequirements);

   VkMemoryAllocateInfo allocInfo ={};
   allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize  = memRequirements.size;
   allocInfo.memoryTypeIndex = vulkanDevice.findMemoryType(memRequirements.memoryTypeBits, properties);

   if(vkAllocateMemory(vulkanDevice.device, &allocInfo, nullptr, imageMemory) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate image memory!");
   }

   vkBindImageMemory(vulkanDevice.device, *image, *imageMemory, 0);
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

   if(vkCreateImageView(vulkanDevice.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
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

// Since this is the camera buffer, this should probably be moved into the camera class
void HelloTriangleApplication::createUniformBuffer()
{
   // camera buffer (view & projection matrices)
   size_t bufferSize = sizeof(camera.getCameraData());

   vulkanDevice.createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &uniformBuffers.cameraBuffer,
      &uniformBuffers.cameraBufferMemory);
}

void HelloTriangleApplication::createDescriptorPool()
{
   mesh->createDescriptorPool();
   std::array<VkDescriptorPoolSize, 1> poolSizes ={};
   poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   poolSizes[0].descriptorCount = 1;
 
   VkDescriptorPoolCreateInfo poolInfo ={};
   poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
   poolInfo.pPoolSizes    = poolSizes.data();
   poolInfo.maxSets       = 1;

   if(vkCreateDescriptorPool(vulkanDevice.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create descriptor pool!");
   }
}

// TODO: These should be moved to respective class.  texture class and camera class.
void HelloTriangleApplication::createDescriptorSet()
{
   VkDescriptorBufferInfo uboBufferInfo ={};
   uboBufferInfo.buffer = uniformBuffers.cameraBuffer;
   uboBufferInfo.offset = 0;
   uboBufferInfo.range  = VK_WHOLE_SIZE;
   
   VkDescriptorSetLayout layoutsMatrixBuffer[] ={ descriptorSetLayoutMatrixBuffer };

   VkDescriptorSetAllocateInfo allocInfoMatrixBuffer ={};
   allocInfoMatrixBuffer.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   allocInfoMatrixBuffer.descriptorPool     = descriptorPool;
   allocInfoMatrixBuffer.descriptorSetCount = 1;
   allocInfoMatrixBuffer.pSetLayouts        = layoutsMatrixBuffer;

   if(vkAllocateDescriptorSets(vulkanDevice.device, &allocInfoMatrixBuffer, &descriptorSetMatrixBuffer) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to allocate MatrixBuffer descriptor set!");
   }

   std::array<VkWriteDescriptorSet, 1> descriptorWritesMatrixBuffer ={};
   descriptorWritesMatrixBuffer[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   descriptorWritesMatrixBuffer[0].dstSet           = descriptorSetMatrixBuffer;
   descriptorWritesMatrixBuffer[0].dstBinding       = 0;
   descriptorWritesMatrixBuffer[0].dstArrayElement  = 0;
   descriptorWritesMatrixBuffer[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   descriptorWritesMatrixBuffer[0].descriptorCount  = 1;
   descriptorWritesMatrixBuffer[0].pBufferInfo      = &uboBufferInfo;
   descriptorWritesMatrixBuffer[0].pImageInfo       = nullptr;
   descriptorWritesMatrixBuffer[0].pTexelBufferView = nullptr;

   vkUpdateDescriptorSets(vulkanDevice.device, static_cast<uint32_t>(descriptorWritesMatrixBuffer.size()), descriptorWritesMatrixBuffer.data(), 0, nullptr);

   mesh->createDescriptorSet();
}

void HelloTriangleApplication::createCommandBuffers()
{
   vulkanStuff.commandBuffers.resize(swapChainFrameBuffers.size());

   VkCommandBufferAllocateInfo allocInfo ={};
   allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool        = vulkanDevice.commandPool;
   allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = (uint32_t)vulkanStuff.commandBuffers.size();

   if(vkAllocateCommandBuffers(vulkanDevice.device, &allocInfo, vulkanStuff.commandBuffers.data()) != VK_SUCCESS)
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

         uint32_t dynamicOffset = j * static_cast<uint32_t>(worldObject->getDynamicAlignment());

         vkCmdBindDescriptorSets(vulkanStuff.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetMatrixBuffer, 0, nullptr);
         vkCmdBindDescriptorSets(vulkanStuff.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, worldObject->getDescriptorSet(), 1, &dynamicOffset);
         vkCmdBindDescriptorSets(vulkanStuff.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, mesh->getDescriptorForMesh(meshId), 0, nullptr);
         
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

   if(vkCreateSemaphore(vulkanDevice.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(vulkanDevice.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create semaphores!");
   }
}

VkFormat HelloTriangleApplication::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
   for(VkFormat format : candidates)
   {
      VkFormatProperties properties;
      vkGetPhysicalDeviceFormatProperties(vulkanDevice.physicalDevice, format, &properties);

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

void HelloTriangleApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
   VkCommandBuffer commandBuffer = vulkanDevice.beginSingleTimeCommand();

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

vulkanDevice.endSingleTimeCommand(commandBuffer);
}

void HelloTriangleApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
   VkCommandBuffer commandBuffer = vulkanDevice.beginSingleTimeCommand();

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

   vulkanDevice.endSingleTimeCommand(commandBuffer);
}

void HelloTriangleApplication::cleanupSwapChain()
{
   for(size_t i=0; i < swapChainFrameBuffers.size(); i++)
   {
      vkDestroyFramebuffer(vulkanDevice.device, swapChainFrameBuffers[i], nullptr);
   }
   vkFreeCommandBuffers(vulkanDevice.device, 
      vulkanDevice.commandPool, 
      static_cast<uint32_t>(vulkanStuff.commandBuffers.size()), 
      vulkanStuff.commandBuffers.data());

   vkDestroyPipeline(vulkanDevice.device, graphicsPipeline, nullptr);
   vkDestroyPipelineLayout(vulkanDevice.device, pipelineLayout, nullptr);
   vkDestroyRenderPass(vulkanDevice.device, renderPass, nullptr);

   for(size_t i=0; i < swapChainImageViews.size(); i++)
   {
      vkDestroyImageView(vulkanDevice.device, swapChainImageViews[i], nullptr);
   }

   vkDestroySwapchainKHR(vulkanDevice.device, swapChain, nullptr);
}

void HelloTriangleApplication::recreateSwapChain()
{
   vkDeviceWaitIdle(vulkanDevice.device);
   cleanupSwapChain();
   
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
   vkFreeMemory(vulkanDevice.device, uniformBuffers.cameraBufferMemory, nullptr);
   vkDestroyBuffer(vulkanDevice.device, uniformBuffers.cameraBuffer, nullptr);
}

void HelloTriangleApplication::createSwapChain()
{
   SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulkanDevice.physicalDevice);

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
   createInfo.surface          = vulkanDevice.surface;
   createInfo.minImageCount    = imageCount;
   createInfo.imageFormat      = surfaceFormat.format;
   createInfo.imageColorSpace  = surfaceFormat.colorSpace;
   createInfo.imageExtent      = extent;
   createInfo.imageArrayLayers = 1;
   createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

   vks::QueueFamilyIndices indices = vulkanDevice.findQueueFamilies();

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

   if(vkCreateSwapchainKHR(vulkanDevice.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create swap chain!");
   }

   vkGetSwapchainImagesKHR(vulkanDevice.device, swapChain, &imageCount, nullptr);

   swapChainImages.resize(imageCount);

   vkGetSwapchainImagesKHR(vulkanDevice.device, swapChain, &imageCount, swapChainImages.data());

   swapChainImageFormat = surfaceFormat.format;
   swapChainExtent      = extent;
}

bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device)
{
   vkGetPhysicalDeviceProperties(device, &vulkanDevice.deviceProperties);
   vkGetPhysicalDeviceFeatures(device, &vulkanDevice.deviceFeatures);

   vks::QueueFamilyIndices indices = vulkanDevice.findQueueFamilies();

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
      vulkanDevice.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
      vulkanDevice.deviceFeatures.geometryShader &&
      vulkanDevice.deviceFeatures.samplerAnisotropy &&
      extensionsSupported &&
      swapChainAdequate;
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

   std::set<std::string> requiredExtensions(vks::deviceExtensions.begin(), vks::deviceExtensions.end());

   for(const auto& extension : availableExtensions)
   {
      requiredExtensions.erase(extension.extensionName);
   }

   return requiredExtensions.empty();
}

HelloTriangleApplication::SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device)
{
   SwapChainSupportDetails details;

   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkanDevice.surface, &details.capabilities);

   uint32_t formatCount;
   vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanDevice.surface, &formatCount, nullptr);

   if(formatCount != 0)
   {
      details.formats.resize(formatCount);

      vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanDevice.surface, &formatCount, details.formats.data());
   }

   uint32_t presentModeCount;
   vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanDevice.surface, &presentModeCount, nullptr);

   if(presentModeCount != 0)
   {
      details.presentModes.resize(presentModeCount);

      vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanDevice.surface, &presentModeCount, details.presentModes.data());
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

   vkDeviceWaitIdle(vulkanDevice.device);
}

void HelloTriangleApplication::updateUniformBuffer()
{
   // TODO: change this stuff, it's weird

   camera.rotate(); // TODO: Should only bew camera->update(), and not called in this method.

   Camera::MatrixBufferObject *mbo = &camera.getCameraData();

   uboVS.projection = mbo->projectionMatrix;
   uboVS.view = mbo->viewMatrix;

   void *data;
   vkMapMemory(vulkanDevice.device, uniformBuffers.cameraBufferMemory, 0, sizeof(uboVS), 0, &data);
   memcpy(data, &uboVS, sizeof(uboVS));
   vkUnmapMemory(vulkanDevice.device, uniformBuffers.cameraBufferMemory);

}


void HelloTriangleApplication::drawFrame()
{
   uint32_t imageIndex;
   VkResult result = vkAcquireNextImageKHR(
      vulkanDevice.device,
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

   if(vkQueueSubmit(vulkanDevice.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
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

   result = vkQueuePresentKHR(vulkanDevice.presentQueue, &presentInfo);

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
      createInfo.enabledLayerCount   = static_cast<uint32_t>(vks::validationLayers.size());
      createInfo.ppEnabledLayerNames = vks::validationLayers.data();
   }
   else
   {
      createInfo.enabledLayerCount = 0;
   }

   if(vkCreateInstance(&createInfo, nullptr, &vulkanDevice.instance) != VK_SUCCESS)
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

   if(CreateDebugReportCallbackEXT(vulkanDevice.instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to set up debug callback!");
   }
}

void HelloTriangleApplication::createSurface()
{
   if(glfwCreateWindowSurface(vulkanDevice.instance, window, nullptr, &vulkanDevice.surface) != VK_SUCCESS)
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

   for(const char* layerName : vks::validationLayers)
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