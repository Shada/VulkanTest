#pragma once
#include "stdafx.h"

enum ShaderType
{
	VERTEX		= VK_SHADER_STAGE_VERTEX_BIT,
	FRAGMENT	= VK_SHADER_STAGE_FRAGMENT_BIT,
	GEOMETRY	= VK_SHADER_STAGE_GEOMETRY_BIT
};

class VulkanShader
{
public:
	void loadShader(const std::string& filename);

	void createShaderModule(VDeleter<VkDevice>& device);

	VkPipelineShaderStageCreateInfo createShaderStage(ShaderType);

private:

	std::vector<char> buffer;

	std::string filename;

	VDeleter<VkShaderModule> shaderModule;
};