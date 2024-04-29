//#define GLM_FORCE_INTRINSICS 

#include <Windows.h>
#include <commdlg.h>

#pragma warning( push )
#pragma warning( disable : 4702) // Disable unreachable code
#define GLFW_INCLUDE_NONE
//#define GLM_FORCE_PURE
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SILENT_WARNINGS

#define MSDFGEN_PUBLIC // ???

#include "Application.h"

//DANGEROUS!
#pragma warning(push, 0)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_write.h>

#define VOLK_IMPLEMENTATION 
#include <Volk/volk.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#pragma warning(pop)

namespace wc
{
	Application app;

	int main()
	{
		Log::Init();
		glfwSetErrorCallback([](int error, const char* description)
			{
				switch (error)
				{
				case GLFW_NOT_INITIALIZED:
					WC_CORE_ERROR("GLFW was not initialized! Description: {0}", description);
					break;
				case GLFW_NO_CURRENT_CONTEXT:
					WC_CORE_ERROR("There is no current GLFW context! Description: {0}", description);
					break;
				case GLFW_INVALID_ENUM:
					WC_CORE_ERROR("GLFW invalid enum! Description: {0}", description);
					break;
				case GLFW_INVALID_VALUE:
					WC_CORE_ERROR("GLFW invalid value! Description: {0}", description);
					break;
				case GLFW_OUT_OF_MEMORY:
					WC_CORE_ERROR("GLFW went out of memory! Description: {0}", description);
					break;
				case GLFW_API_UNAVAILABLE:
					WC_CORE_ERROR("GLFW API is not available! Description: {0}", description);
					break;
				case GLFW_VERSION_UNAVAILABLE:
					WC_CORE_ERROR("GLFW version is not available! Description: {0}", description);
					break;
				case GLFW_PLATFORM_ERROR:
					WC_CORE_ERROR("GLFW platform error! Description: {0}", description);
					break;
				case GLFW_FORMAT_UNAVAILABLE:
					WC_CORE_ERROR("GLFW format is not available! Description: {0}", description);
					break;
				}
			});
		glfwInit();
		if (glfwVulkanSupported())
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			VK_CHECK(volkInitialize());
			app.Start();
		}
		else
			WC_CORE_ERROR("Vulkan driver is not supported!");

		glfwTerminate();

		return 0;
	}
}

int main()
{
	return wc::main();
}