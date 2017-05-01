# VulkanTest
Application for testing Vulkan and its features

This is a small created only for testing Vulkan and it's features. 

Most of the content is derived from following tutorials such as the one found on https://vulkan-tutorial.com/

To compile and run the project, you need GLFW and VulkanSDK

I have compiled GLFW 3.2.1 for Visual Studio 2017 and included under the externals folder together with the 
pre-compiled versions for vc-2010 to vc2015. The pre-compiled GLFW can also be found on the GLFW download page 
(http://www.glfw.org/download.html) together with the source code if you like to compile the library yourself.

To compile and run the application you need to have installed the VulkanSDK. The Visual Studio Project is expecting 
the VulkanSDK to be located at "C:\VulkanSDK". If you install the SDK on another location, you need change the linker 
location in the Project Properties.
