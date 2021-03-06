#include "VulkanShader.h"
#include <fstream>

void VulkanShader::loadShader(const std::string &filename)
{
   std::ifstream file(filename, std::ios::ate | std::ios::binary);

   if(!file.is_open())
   {
      throw std::runtime_error("failed to open file " + filename + "!");
   }

   this->filename = filename;

   size_t fileSize = static_cast<size_t>(file.tellg());
   buffer.resize(fileSize);

   file.seekg(0);
   file.read(buffer.data(), fileSize);

   file.close();
}

//TODO: we only have one shader module... so why do we store it as a vector?
// what is it used for?
void VulkanShader::createShaderModule(VkDevice &device)
{
   std::vector<uint32_t> codeAlligned(buffer.size() / sizeof(uint32_t) + 1);
   memcpy(codeAlligned.data(), buffer.data(), buffer.size());

   VkShaderModuleCreateInfo createInfo ={};

   createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = buffer.size();
   createInfo.pCode    = codeAlligned.data();

   if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
   {
      throw std::runtime_error("failed to create shader module " + filename + "!");
   }
}

VkPipelineShaderStageCreateInfo VulkanShader::createShaderStage(ShaderType shaderType)
{
   VkPipelineShaderStageCreateInfo shaderStageInfo ={};

   shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   shaderStageInfo.stage = (VkShaderStageFlagBits)shaderType;

   shaderStageInfo.module = shaderModule;
   shaderStageInfo.pName  = "main";

   return shaderStageInfo;
}