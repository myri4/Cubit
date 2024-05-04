#pragma once

#pragma warning(push, 0)
#define VK_NO_PROTOTYPES
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include <Volk/volk.h>
#include <GLFW/glfw3.h>
#include <vma/vk_mem_alloc.h>

#pragma warning(pop)
#include <magic_enum.hpp>
#include <unordered_set>
#include <glm/glm.hpp>
#include "../Utils/Log.h"

#define WC_GRAPHICS_DEBUGGER 0
#define WC_SHADER_DEBUG_PRINT 0

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			WC_CORE_ERROR("{0} returned: {1}", #x, std::string(magic_enum::enum_name((VkResult)err))); \
			abort();                                                \
		}                                                           \
	} while (0)

template<class T>
VkObjectType GetObjectType()
{
	if (typeid(T) == typeid(VkInstance)) return VK_OBJECT_TYPE_INSTANCE;
	if (typeid(T) == typeid(VkPhysicalDevice)) return VK_OBJECT_TYPE_PHYSICAL_DEVICE;
	if (typeid(T) == typeid(VkDevice)) return VK_OBJECT_TYPE_DEVICE;
	if (typeid(T) == typeid(VkQueue)) return VK_OBJECT_TYPE_QUEUE;
	if (typeid(T) == typeid(VkSemaphore)) return VK_OBJECT_TYPE_SEMAPHORE;
	if (typeid(T) == typeid(VkCommandBuffer)) return VK_OBJECT_TYPE_COMMAND_BUFFER;
	if (typeid(T) == typeid(VkFence)) return VK_OBJECT_TYPE_FENCE;
	if (typeid(T) == typeid(VkDeviceMemory)) return VK_OBJECT_TYPE_DEVICE_MEMORY;
	if (typeid(T) == typeid(VkBuffer)) return VK_OBJECT_TYPE_BUFFER;
	if (typeid(T) == typeid(VkImage)) return VK_OBJECT_TYPE_IMAGE;
	if (typeid(T) == typeid(VkEvent)) return VK_OBJECT_TYPE_EVENT;
	if (typeid(T) == typeid(VkQueryPool)) return VK_OBJECT_TYPE_QUERY_POOL;
	if (typeid(T) == typeid(VkBufferView)) return VK_OBJECT_TYPE_BUFFER_VIEW;
	if (typeid(T) == typeid(VkImageView)) return VK_OBJECT_TYPE_IMAGE_VIEW;
	if (typeid(T) == typeid(VkShaderModule)) return VK_OBJECT_TYPE_SHADER_MODULE;
	if (typeid(T) == typeid(VkPipelineCache)) return VK_OBJECT_TYPE_PIPELINE_CACHE;
	if (typeid(T) == typeid(VkPipelineLayout)) return VK_OBJECT_TYPE_PIPELINE_LAYOUT;
	if (typeid(T) == typeid(VkRenderPass)) return VK_OBJECT_TYPE_RENDER_PASS;
	if (typeid(T) == typeid(VkPipeline)) return VK_OBJECT_TYPE_PIPELINE;
	if (typeid(T) == typeid(VkDescriptorSetLayout)) return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
	if (typeid(T) == typeid(VkSampler)) return VK_OBJECT_TYPE_SAMPLER;
	if (typeid(T) == typeid(VkDescriptorPool)) return VK_OBJECT_TYPE_DESCRIPTOR_POOL;
	if (typeid(T) == typeid(VkDescriptorSet)) return VK_OBJECT_TYPE_DESCRIPTOR_SET;
	if (typeid(T) == typeid(VkFramebuffer)) return VK_OBJECT_TYPE_FRAMEBUFFER;
	if (typeid(T) == typeid(VkCommandPool)) return VK_OBJECT_TYPE_COMMAND_POOL;

	return VK_OBJECT_TYPE_UNKNOWN;
}

namespace VulkanContext
{
	void SetObjectName(VkObjectType object_type, uint64_t object_handle, const char* object_name);
}

template <class T>
class VkObject
{
protected:
	T m_RendererID = VK_NULL_HANDLE;
public:
	VkObject() = default;
	VkObject(T handle) { m_RendererID = handle; }

	operator T& () { return m_RendererID; }
	operator const T& () const { return m_RendererID; }
	operator bool() const { return m_RendererID != VK_NULL_HANDLE; }

	VkObjectType GetType() const { return GetObjectType<T>(); }

	void SetName(const char* name) const { VulkanContext::SetObjectName(GetType(), (uint64_t)m_RendererID, name); }
	void SetName(const std::string& name) const { VulkanContext::SetObjectName(GetType(), (uint64_t)m_RendererID, name.c_str()); }

	T* GetPointer() { return &m_RendererID; }
	const T* GetPointer() const { return &m_RendererID; }
};

namespace wc // We use WC namespace for simplicity
{
	inline size_t align(size_t originalSize, size_t alignment)
	{
		size_t alignedSize = originalSize;
		if (alignment > 0)
			alignedSize = (alignedSize + alignment - 1) & ~(alignment - 1);

		return alignedSize;
	}

	class PhysicalDevice : public VkObject<VkPhysicalDevice>
	{
		VkPhysicalDeviceFeatures features = {};
		VkPhysicalDeviceProperties properties = {};
		VkPhysicalDeviceProperties2 properties2 = {};
		VkPhysicalDeviceMemoryProperties memoryProperties = {};
	public:

		PhysicalDevice() = default;
		PhysicalDevice(VkPhysicalDevice device) { SetDevice(device); }

		void SetDevice(VkPhysicalDevice physicalDevice)
		{
			m_RendererID = physicalDevice;
			vkGetPhysicalDeviceFeatures(m_RendererID, &features);
			vkGetPhysicalDeviceProperties(m_RendererID, &properties);
			vkGetPhysicalDeviceMemoryProperties(m_RendererID, &memoryProperties);
		}

		VkResult GetImageFormatProperties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
		{
			return vkGetPhysicalDeviceImageFormatProperties(m_RendererID, format, type, tiling, usage, flags, pImageFormatProperties);
		}

		void GetQueueFamilyProperties(uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties)
		{
			vkGetPhysicalDeviceQueueFamilyProperties(m_RendererID, pQueueFamilyPropertyCount, pQueueFamilyProperties);
		}

		VkFormatProperties GetFormatProperties(VkFormat format)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(m_RendererID, format, &formatProperties);
			return formatProperties;
		}

		void QueryProperties2(void* pNext = nullptr)
		{
			properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			properties2.pNext = pNext;
			vkGetPhysicalDeviceProperties2(m_RendererID, &properties2);
		}

		VkPhysicalDeviceFeatures GetFeatures() const { return features; }

		VkPhysicalDeviceProperties GetProperties() const { return properties; }
		VkPhysicalDeviceProperties2 GetProperties2() const { return properties2; }

		VkPhysicalDeviceMemoryProperties GetMemoryProperties() const { return memoryProperties; }
	};


	struct LogicalDevice : public VkObject<VkDevice> 
	{
		VkResult Create(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& createInfo) { return vkCreateDevice(physicalDevice, &createInfo, nullptr, &m_RendererID); }

		PFN_vkVoidFunction GetProcAddress(const char* pName) { return vkGetDeviceProcAddr(m_RendererID, pName); }

		void WaitIdle()	{ vkDeviceWaitIdle(m_RendererID); }

		void Destroy() { vkDestroyDevice(m_RendererID, nullptr); }
	};
}

namespace VulkanContext
{
		inline VkInstance instance;
		inline wc::PhysicalDevice physicalDevice;
		inline wc::LogicalDevice device;

		inline VmaAllocator vmaAllocator;

#if WC_GRAPHICS_DEBUGGER
		inline bool bValidationLayers = false;
		inline VkDebugUtilsMessengerEXT debug_messenger;
		inline const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

		VKAPI_ATTR inline VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{

			switch (messageSeverity)
			{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				WC_CORE_ERROR(pCallbackData->pMessage);
				//WC_DEBUGBREAK();
				//OutputDebugString(pCallbackData->pMessage);
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				WC_CORE_WARN(pCallbackData->pMessage);
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				WC_CORE_TRACE(pCallbackData->pMessage);
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				//WC_CORE_TRACE(pCallbackData->pMessage);
				break;
			}

			switch (messageType)
			{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
				WC_CORE_WARN("Performance: {0}", pCallbackData->pMessage);
				//OutputDebugString(pCallbackData->pMessage);
				break;

			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
				//WC_CORE_TRACE("General: {0}", pCallbackData->pMessage);
				break;
			}

			return true;
		}

		inline void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
		{
			createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

#if WC_SHADER_DEBUG_PRINT
			createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
#endif

			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = DebugCallback;
		}
#endif


	inline VkInstance& GetInstance() { return instance; }
	inline wc::PhysicalDevice& GetPhysicalDevice() { return physicalDevice; }
	inline wc::LogicalDevice& GetLogicalDevice() { return device; }
	inline VmaAllocator& GetMemoryAllocator() { return vmaAllocator; }
	inline VkAllocationCallbacks* GetAllocator() { return nullptr; }
	inline VkPhysicalDeviceProperties GetProperties() { return physicalDevice.GetProperties(); }
	inline VkPhysicalDeviceFeatures GetSupportedFeatures() { return physicalDevice.GetFeatures(); }

	inline void BeginLabel(VkCommandBuffer command_buffer, const char* label_name, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = label_name;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
		}
#endif
	}

	inline void InsertLabel(VkCommandBuffer command_buffer, const char* label_name, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = label_name;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label);
		}
#endif
	}

	inline void EndLabel(VkCommandBuffer command_buffer) 
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers) vkCmdEndDebugUtilsLabelEXT(command_buffer);
#endif
	}


	inline void BeginLabel(VkQueue queue, const char* label_name, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = label_name;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkQueueBeginDebugUtilsLabelEXT(queue, &label);
		}
#endif
	}

	inline void InsertLabel(VkQueue queue, const char* label_name, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = label_name;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkQueueInsertDebugUtilsLabelEXT(queue, &label);
		}
#endif
	}

	inline void EndLabel(VkQueue queue) 
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers) vkQueueEndDebugUtilsLabelEXT(queue);
#endif
	}

	inline void SetObjectName(VkObjectType object_type, uint64_t object_handle, const char* object_name)
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers)
		{
			VkDebugUtilsObjectNameInfoEXT name_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
			name_info.objectType = object_type;
			name_info.objectHandle = object_handle;
			name_info.pObjectName = object_name;
			vkSetDebugUtilsObjectNameEXT(device, &name_info);
		}
#endif
	}

	template<class T>
	inline void SetObjectName(T object_handle, const char* object_name)
	{
#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers)
		{
			VkDebugUtilsObjectNameInfoEXT name_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
			name_info.objectType = GetObjectType<T>();
			name_info.objectHandle = (uint64_t)object_handle;
			name_info.pObjectName = object_name;
			vkSetDebugUtilsObjectNameEXT(device, &name_info);
		}
#endif
	}


	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		//std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> computeFamily;
		std::optional<uint32_t> transferFamily;

		bool isComplete() {
			return
				graphicsFamily.has_value() &&
				//presentFamily.has_value() &&
				computeFamily.has_value() &&
				transferFamily.has_value();
		}
	};


	inline QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physDevice/*, VkSurfaceKHR surface*/)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

		for (int i = 0; i < queueFamilies.size(); i++) 
		{
			const auto& queueFamily = queueFamilies[i];
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) 	indices.computeFamily = i;
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) indices.transferFamily = i;

			if (indices.isComplete())
				return indices;
		}

		return indices;
	}

	inline bool isDeviceSuitable(VkPhysicalDevice physDevice, const std::vector<const char*>& deviceExtensions/*, VkSurfaceKHR surface*/)
	{
		QueueFamilyIndices indices = findQueueFamilies(physDevice/*, surface*/);

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions.data());

		std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);

		bool extensionsSupported = requiredExtensions.empty();

		//bool swapChainAdequate = false;
		//if (extensionsSupported) {
		//	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		//	swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		//}

		return indices.isComplete() && extensionsSupported /*&& swapChainAdequate*/;
	}

	class Queue : public VkObject<VkQueue>
	{
		uint32_t queueFamily = 0;
	public:

		void GetDeviceQueue(uint32_t family) 
		{
			queueFamily = family;
			vkGetDeviceQueue(device, family, 0, &m_RendererID);
		}

		VkResult Submit(const VkSubmitInfo& submit_info, VkFence fence = VK_NULL_HANDLE) const { return vkQueueSubmit(m_RendererID, 1, &submit_info, fence); }

		VkResult PresentKHR(const VkPresentInfoKHR& present_info) const { return vkQueuePresentKHR(m_RendererID, &present_info); }

		void WaitIdle() const { vkQueueWaitIdle(m_RendererID); }

		uint32_t GetFamily() const { return queueFamily; }
	};

	inline Queue GraphicsQueue;
	//inline Queue PresentQueue;
	inline Queue ComputeQueue;
	inline Queue TransferQueue;

	inline void Create()
	{
		{// Create Instance
#if WC_GRAPHICS_DEBUGGER
			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : validationLayers)
				for (const auto& layerProperties : availableLayers)
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						bValidationLayers = true;
						break;
					}

			if (!bValidationLayers)
				WC_CORE_ERROR("Validation layers requested, but not available!");
#endif

			VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
			appInfo.pApplicationName = "WC Application";
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
			appInfo.pEngineName = "WC Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
			appInfo.apiVersion = VK_API_VERSION_1_2;

			VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
			instanceCreateInfo.pApplicationInfo = &appInfo;

			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if WC_GRAPHICS_DEBUGGER
			if (bValidationLayers) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);

			VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
			VkValidationFeatureEnableEXT enabledFeatures[] = {
				VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
				//VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
				//VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT
			};
			validationFeatures.pEnabledValidationFeatures = enabledFeatures;
			validationFeatures.enabledValidationFeatureCount = (uint32_t)std::size(enabledFeatures);

			debugCreateInfo.pNext = (VkValidationFeaturesEXT*)&validationFeatures;

			instanceCreateInfo.pNext = bValidationLayers ? (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo : nullptr;
#endif
			instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
			instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

			if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
				WC_CORE_ERROR("Failed to create instance!");

			volkLoadInstance(instance);
		}

#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers)
		{
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
			populateDebugMessengerCreateInfo(debugCreateInfo);

			if (vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, VulkanContext::GetAllocator(), &debug_messenger) != VK_SUCCESS)
				WC_CORE_ERROR("Failed to set up debug messenger!");
		}
#endif

		const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if WC_GRAPHICS_DEBUGGER

#if WC_SHADER_DEBUG_PRINT
			VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
#endif

#endif
		};

		{ // Pick physical device 
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0)
				WC_CORE_ERROR("failed to find GPUs with Vulkan support!");


			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			for (const auto& currentDevice : devices) 
			{
				if (isDeviceSuitable(currentDevice, deviceExtensions)) 
				{
					physicalDevice = currentDevice;
					break;
				}
			}

			if (physicalDevice == VK_NULL_HANDLE)
				WC_CORE_ERROR("failed to find a suitable GPU!");
		}
		{ // Create Logical Device
			QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::unordered_set<uint32_t> uniqueQueueFamilies = {
				indices.graphicsFamily.value(),
				//indices.presentFamily.value(),
				indices.computeFamily.value(),
				indices.transferFamily.value(),
			};

			float queuePriorities[] = { 1.f };
			for (uint32_t queueFamily : uniqueQueueFamilies) 
			{
				VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = (uint32_t)std::size(queuePriorities);
				queueCreateInfo.pQueuePriorities = queuePriorities;

				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures{};
			if (physicalDevice.GetFeatures().samplerAnisotropy) deviceFeatures.samplerAnisotropy = true;

			VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

			features12.shaderSampledImageArrayNonUniformIndexing = true;
			features12.runtimeDescriptorArray = true;
			features12.descriptorBindingVariableDescriptorCount = true;
			features12.descriptorBindingPartiallyBound = true;
			features12.bufferDeviceAddress = true;

			VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };

			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();

			createInfo.pEnabledFeatures = &deviceFeatures;

			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			createInfo.pNext = &features12;

#if WC_GRAPHICS_DEBUGGER
			if (bValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
#endif

			if (device.Create(physicalDevice, createInfo) != VK_SUCCESS)
				WC_CORE_ERROR("Failed to create logical device!");

			GraphicsQueue.GetDeviceQueue(indices.graphicsFamily.value());
			//presentQueue.GetDeviceQueue(indices.presentFamily.value());
			ComputeQueue.GetDeviceQueue(indices.computeFamily.value());
			TransferQueue.GetDeviceQueue(indices.transferFamily.value());

			GraphicsQueue.SetName("GraphicsQueue");
			ComputeQueue.SetName("ComputeQueue");
			TransferQueue.SetName("TransferQueue");
		}

		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &vmaAllocator);
	}

	inline void Destroy()
	{
		vmaDestroyAllocator(vmaAllocator);
		device.Destroy();

#if WC_GRAPHICS_DEBUGGER
		if (bValidationLayers) vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, VulkanContext::GetAllocator());
#endif
		vkDestroyInstance(instance, nullptr);
	}
}

namespace wc
{
	using Queue = VulkanContext::Queue;
}