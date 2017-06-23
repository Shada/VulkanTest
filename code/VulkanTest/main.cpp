#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h> 

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "VulkanTestApplication.h"

#include <iostream>

int main()
{
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
   _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);

   HelloTriangleApplication app;

   try
   {
      app.run();
   }
   catch(const std::runtime_error& e)
   {
      std::cerr
         << e.what()
         << std::endl;

      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}