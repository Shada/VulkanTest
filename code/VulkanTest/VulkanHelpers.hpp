#pragma once
#include <vulkan\vulkan.hpp>

namespace vkn
{
   namespace inits
   {
      inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
         uint32_t binding, 
         VkDescriptorType descriptorType, 
         VkShaderStageFlags shaderStageFlags)
      {
         VkDescriptorSetLayoutBinding descriptorSetLayoutBinding ={};

         descriptorSetLayoutBinding.binding            = binding;
         descriptorSetLayoutBinding.descriptorType     = descriptorType;
         descriptorSetLayoutBinding.descriptorCount    = 1;
         descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
         descriptorSetLayoutBinding.stageFlags         = shaderStageFlags;

         return descriptorSetLayoutBinding;
      }

      inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
         VkDescriptorSetLayoutBinding binding)
      {
         VkDescriptorSetLayoutCreateInfo layoutInfo ={};

         layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = 1;
         layoutInfo.pBindings    = &binding;

         return layoutInfo;
      }

      inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
         std::vector<VkDescriptorSetLayoutBinding> bindings)
      {
         VkDescriptorSetLayoutCreateInfo layoutInfo ={};

         layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
         layoutInfo.pBindings    = bindings.data();

         return layoutInfo;
      }
      
      inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
         const VkDescriptorSetLayoutBinding* bindings, 
         uint32_t bindingCount)
      {
         VkDescriptorSetLayoutCreateInfo layoutInfo ={};

         layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
         layoutInfo.bindingCount = bindingCount;
         layoutInfo.pBindings    = bindings;

         return layoutInfo;
      }

      inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
         uint32_t poolSizeCount,
         VkDescriptorPoolSize *poolSizes,
         uint32_t maxSets
      )
      {
         VkDescriptorPoolCreateInfo poolCreateInfo={};

         poolCreateInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
         poolCreateInfo.maxSets        = maxSets;
         poolCreateInfo.poolSizeCount  = poolSizeCount;
         poolCreateInfo.pPoolSizes     = poolSizes;

         return poolCreateInfo;
      }
   };

};