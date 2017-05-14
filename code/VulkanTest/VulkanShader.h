#pragma once
#include "stdafx.h"

class VulkanShader
{
public:
	void loadShader(const std::string& filename);

	void createShaderModule(VDeleter<VkDevice>& device);

private:

	std::vector<char> buffer;
	
	std::string filename;

	VDeleter<VkShaderModule> shaderModule;
};

