#pragma once

#pragma warning(push, 0)
#include <magic_enum.hpp>
#pragma warning(pop)

#include "vk/Buffer.h"
#include "vk/Pipeline.h"
#include "vk/Descriptors.h"

#include <fstream>
#include <filesystem>
#include <spirv_cross/spirv_cross.hpp>

namespace wc 
{
	inline std::vector<uint32_t> ReadBinary(const std::string& filename) 
	{
		std::vector<uint32_t> buffer;
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			WC_CORE_ERROR("Cant open file at location {0}", filename.c_str());

		//find what the size of the file is by looking up the location of the cursor
		//because the cursor is at the end, it gives the size directly in bytes
		size_t fileSize = (size_t)file.tellg();

		//spirv expects the buffer to be on uint32, so make sure to reserve a int vector big enough for the entire file
		buffer.resize(fileSize / sizeof(uint32_t));

		//put file cursor at beggining
		file.seekg(0);

		//load the entire file into the buffer
		file.read((char*)buffer.data(), fileSize);

		//now that the file is loaded into the buffer, we can close it
		file.close();
		return buffer;
	}

	struct ShaderCreateInfo 
	{
		std::string vertexShader;
		std::string fragmentShader;
		std::string cachePath;

		glm::vec2 renderSize = glm::vec2(0.f);
		VkRenderPass renderPass;

		bool blending = false;
		bool depthTest = true;

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkDescriptorBindingFlags* bindingFlags = nullptr;
		uint32_t bindingFlagCount = 0;
		uint32_t dynamicDescriptorCount = 0;

		VkDynamicState* dynamicState = nullptr;
		uint32_t dynamicStateCount = 0;
	};

	class Shader
	{
		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
		DescriptorSetLayout m_DescriptorLayout;
		//PipelineCache m_Cache;
		//std::string m_CachePath;
	public:
		VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		DescriptorSetLayout GetDescriptorLayout() const { return m_DescriptorLayout; }
		//PipelineCache GetCache() const { return m_Cache; }

		void Create(const ShaderCreateInfo& createInfo) 
		{
			std::array<VkShaderModule, 2> shaderModules = {};
			std::vector<uint32_t> binaries[2] = {};
			std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};

			//m_CachePath = createInfo.cachePath;
			//bool cacheFound = std::filesystem::exists(m_CachePath);
			VkPipelineCacheCreateInfo cacheCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };

			{
				auto& binary = binaries[0];
				binary = ReadBinary(createInfo.vertexShader);
				VkShaderModuleCreateInfo moduleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

				moduleCreateInfo.codeSize = binary.size() * sizeof(uint32_t);
				moduleCreateInfo.pCode = binary.data();

				vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &moduleCreateInfo, nullptr, &shaderModules[0]);
			}
			{
				auto& binary = binaries[1];
				binary = ReadBinary(createInfo.fragmentShader);
				VkShaderModuleCreateInfo moduleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

				moduleCreateInfo.codeSize = binary.size() * sizeof(uint32_t);
				moduleCreateInfo.pCode = binary.data();

				vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &moduleCreateInfo, nullptr, &shaderModules[1]);
			}
			//if (cacheFound) {
			//	auto cacheData = ReadBinary(m_CachePath);
			//	cacheCreateInfo.pInitialData = cacheData.data();
			//	cacheCreateInfo.initialDataSize = cacheData.size() * sizeof(uint32_t);
			//
			//	if (!m_Cache.Valid(cacheData.data())) {
			//		WC_CORE_WARN("Invalid cache file for {}. Ignoring and creating a new one", createInfo.cachePath.c_str());
			//		cacheCreateInfo.pInitialData = nullptr;
			//		cacheCreateInfo.initialDataSize = 0;
			//	}
			//}
			//else
			//	WC_INFO("Could not find cache file for {}", m_CachePath.c_str());

			{ // Reflection
				std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
				std::vector<VkPushConstantRange> ranges;

				for (uint32_t i = 0; i < shaderModules.size(); i++) 
				{
					spirv_cross::Compiler compiler(binaries[i]);
					spirv_cross::ShaderResources resources = compiler.get_shader_resources();

					spirv_cross::EntryPoint entryPoint = compiler.get_entry_points_and_stages()[0];
					VkShaderStageFlags shaderStage = VkShaderStageFlagBits(1 << entryPoint.execution_model);

					for (auto& resource : resources.push_constant_buffers) 
					{
						auto& baseType = compiler.get_type(resource.base_type_id);
						uint32_t bufferSize = (uint32_t)compiler.get_declared_struct_size(baseType);

						uint32_t offset = 0;
						if (ranges.size())
							offset = ranges.back().offset + ranges.back().size;

						auto& pushConstantRange = ranges.emplace_back();
						pushConstantRange.stageFlags = shaderStage;
						pushConstantRange.size = bufferSize;
						pushConstantRange.offset = 0;
					}


					for (auto& resource : resources.uniform_buffers) 
					{
						bool add = true;
						VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
						for (auto& layoutBinding : layoutBindings) 
							if (layoutBinding.descriptorType == descriptorType && layoutBinding.binding == binding)
							{
								add = false;
								layoutBinding.stageFlags |= shaderStage;
								break;
							}						

						if (add) 
						{
							auto& layoutBinding = layoutBindings.emplace_back();

							const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							if (type.array[0] > 0) descriptorCount = type.array[0];

							layoutBinding.descriptorType = descriptorType;
							layoutBinding.descriptorCount = descriptorCount;
							layoutBinding.stageFlags = shaderStage;
							layoutBinding.binding = binding;
							layoutBinding.pImmutableSamplers = nullptr;
						}
					}

					for (auto& resource : resources.storage_buffers) 
					{
						bool add = true;
						VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
						for (auto& layoutBinding : layoutBindings) 
							if (layoutBinding.descriptorType == descriptorType && layoutBinding.binding == binding)
							{
								add = false;
								layoutBinding.stageFlags |= shaderStage;
								break;
							}						

						if (add) 
						{
							auto& layoutBinding = layoutBindings.emplace_back();

							//const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							// @TODO: report this as an issue with the new spirv-cross reflection as this is probably a nullptr and gives random garbage which is not supposed to happen
							//if (type.array[0] > 0) descriptorCount = type.array[0]; 

							layoutBinding.descriptorType = descriptorType;
							layoutBinding.descriptorCount = descriptorCount;
							layoutBinding.stageFlags = shaderStage;
							layoutBinding.binding = binding;
							layoutBinding.pImmutableSamplers = nullptr;
						}
					}

					if (shaderStage == VK_SHADER_STAGE_FRAGMENT_BIT) 
					{
						for (auto& resource : resources.storage_images) 
						{
							auto& layoutBinding = layoutBindings.emplace_back();

							const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							//if (type.array[0] > 0) descriptorCount = type.array[0];

							layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
							layoutBinding.descriptorCount = descriptorCount;
							layoutBinding.stageFlags = shaderStage;
							layoutBinding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
							layoutBinding.pImmutableSamplers = nullptr;
						}

						for (auto& resource : resources.sampled_images) 
						{
							auto& layoutBinding = layoutBindings.emplace_back();

							const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							//if (type.array[0] > 0) descriptorCount = type.array[0];
							
							layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
							layoutBinding.descriptorCount = descriptorCount;
							layoutBinding.stageFlags = shaderStage;
							layoutBinding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
							layoutBinding.pImmutableSamplers = nullptr;
						}
					}

					VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

					stage.stage = (VkShaderStageFlagBits)shaderStage;
					stage.module = shaderModules[i];
					stage.pName = "main";
					shaderStages[i] = stage;
				}

				if (createInfo.dynamicDescriptorCount)
					layoutBindings[layoutBindings.size() - 1].descriptorCount = createInfo.dynamicDescriptorCount;

				VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };

				layoutInfo.pBindings = layoutBindings.data();
				layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());

				VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };

				if (createInfo.bindingFlagCount > 0) 
				{
					binding_flags.bindingCount = createInfo.bindingFlagCount;
					binding_flags.pBindingFlags = createInfo.bindingFlags;

					layoutInfo.pNext = &binding_flags;
				}

				m_DescriptorLayout.Create(layoutInfo);

				VkPipelineLayoutCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

				info.flags = 0;
				info.setLayoutCount = 1;
				info.pSetLayouts = m_DescriptorLayout.GetPointer();
				info.pPushConstantRanges = ranges.data();
				info.pushConstantRangeCount = (uint32_t)ranges.size();

				vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &info, VulkanContext::GetAllocator(), &m_PipelineLayout);
			}

			VkViewport viewport = { 0.f };
			VkRect2D scissor = {};

			viewport.x = 0.f;
			viewport.y = 0.f;
			//viewport.y = createInfo.renderSize.y; // change this to 0 to invert
			viewport.width = createInfo.renderSize.x;
			viewport.height = createInfo.renderSize.y;
			//viewport.height = -createInfo.renderSize.y; // remove the - to invert
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			scissor.offset = { 0, 0 };
			scissor.extent = { (uint32_t)createInfo.renderSize.x, (uint32_t)createInfo.renderSize.y };

			//a single blend attachment with no blending and writing to RGBA
			VkPipelineColorBlendAttachmentState colorBlend = {};
			colorBlend.blendEnable = createInfo.blending;
			colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

			colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;

			colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

			depthStencil.depthTestEnable = createInfo.depthTest;
			depthStencil.depthWriteEnable = createInfo.depthTest; // should be depth write
			depthStencil.depthCompareOp = createInfo.depthTest ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS; // should be changeable
			depthStencil.depthBoundsTestEnable = false;
			depthStencil.minDepthBounds = 0.f; // Optional
			depthStencil.maxDepthBounds = 1.f; // Optional
			depthStencil.stencilTestEnable = false;


			//m_Cache.Create(cacheCreateInfo);

			VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

			colorBlending.logicOpEnable = false;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlend;
			colorBlending.blendConstants[0] = 1.f;
			colorBlending.blendConstants[1] = 1.f;
			colorBlending.blendConstants[2] = 1.f;
			colorBlending.blendConstants[3] = 1.f;

			VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

			pipelineInfo.stageCount = (uint32_t)shaderStages.size();
			pipelineInfo.pStages = shaderStages.data();

			VkPipelineVertexInputStateCreateInfo verttexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
			pipelineInfo.pVertexInputState = &verttexInputState;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

			inputAssembly.topology = createInfo.topology;
			inputAssembly.primitiveRestartEnable = false;
			pipelineInfo.pInputAssemblyState = &inputAssembly;

			pipelineInfo.pViewportState = &viewportState;

			VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

			rasterizer.depthClampEnable = false;
			rasterizer.rasterizerDiscardEnable = false;

			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.f;
			rasterizer.cullMode = VK_CULL_MODE_NONE;
			rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = false;
			rasterizer.depthBiasConstantFactor = 0.f;
			rasterizer.depthBiasClamp = 0.f;
			rasterizer.depthBiasSlopeFactor = 0.f;
			pipelineInfo.pRasterizationState = &rasterizer;

			VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

			multisampling.sampleShadingEnable = false;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.minSampleShading = 1.f;
			multisampling.pSampleMask = nullptr;
			multisampling.alphaToCoverageEnable = false;
			multisampling.alphaToOneEnable = false;
			pipelineInfo.pMultisampleState = &multisampling;

			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.layout = m_PipelineLayout;
			pipelineInfo.renderPass = createInfo.renderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = nullptr;

			pipelineInfo.pDepthStencilState = &depthStencil;

			VkPipelineDynamicStateCreateInfo dynamicState = {};
			if (createInfo.dynamicStateCount)
			{
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.dynamicStateCount = createInfo.dynamicStateCount;
				dynamicState.pDynamicStates = createInfo.dynamicState;
				pipelineInfo.pDynamicState = &dynamicState;
			}

			vkCreateGraphicsPipelines(VulkanContext::GetLogicalDevice(), /*m_Cache*/VK_NULL_HANDLE, 1, &pipelineInfo, VulkanContext::GetAllocator(), &m_Pipeline);

			//if (!cacheFound) 
			for (uint32_t i = 0; i < shaderModules.size(); i++) vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModules[i], nullptr);
		}

		void Bind(CommandBuffer cmd) { vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);	}

		void SaveCache() 
		{
#if DISABLE_CACHING == 0 // @TODO(myri4): need to add it above
			//cache.SaveToFile(cachePath);
#endif
		}

		void Destroy() 
		{
			vkDestroyPipeline(VulkanContext::GetLogicalDevice(), m_Pipeline, VulkanContext::GetAllocator());
			m_Pipeline = VK_NULL_HANDLE;

			vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), m_PipelineLayout, VulkanContext::GetAllocator());
			m_PipelineLayout = VK_NULL_HANDLE;

			//m_Cache.Destroy();
			m_DescriptorLayout.Destroy();
		}
	};

	struct ComputeShaderCreateInfo 
	{
		std::string path;
		//std::string infoPath;
		std::string cachePath;

		VkDescriptorBindingFlags* bindingFlags = nullptr;
		uint32_t bindingFlagCount = 0;
		uint32_t dynamicDescriptorCount = 0;
	};

	class ComputeShader 
	{
		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
		DescriptorSetLayout m_DescriptorLayout;
	public:
		VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		DescriptorSetLayout GetDescriptorLayout() const { return m_DescriptorLayout; }


		void Create(const ComputeShaderCreateInfo& createInfo) {
			VkShaderModule shaderModule;

				auto binary = ReadBinary(createInfo.path);
			{
				VkShaderModuleCreateInfo moduleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

				moduleCreateInfo.codeSize = binary.size() * sizeof(uint32_t);
				moduleCreateInfo.pCode = binary.data();

				vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &moduleCreateInfo, nullptr, &shaderModule);
			}

			//if (createInfo.infoPath.size() == 0) WC_CORE_WARN("Shader info for shader with path {} is empty!", createInfo.path);

			{ // Reflection

				std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

				spirv_cross::Compiler compiler(binary);
				spirv_cross::ShaderResources resources = compiler.get_shader_resources();
				VkShaderStageFlags shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;

				for (auto& resource : resources.uniform_buffers) {
					bool add = true;
					VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					for (auto& layoutBinding : layoutBindings) 
						if (layoutBinding.descriptorType == descriptorType && layoutBinding.binding == binding)
						{
							add = false;
							layoutBinding.stageFlags |= shaderStage;
							break;
						}					

					if (add) 
					{
						auto& layoutBinding = layoutBindings.emplace_back();

						const auto& type = compiler.get_type(resource.type_id);

						uint32_t descriptorCount = 1;
						//if (type.array[0] > 0) descriptorCount = type.array[0];

						layoutBinding.descriptorType = descriptorType;
						layoutBinding.descriptorCount = descriptorCount;
						layoutBinding.stageFlags = shaderStage;
						layoutBinding.binding = binding;
						layoutBinding.pImmutableSamplers = nullptr;
					}
				}

				for (auto& resource : resources.storage_buffers) {
					bool add = true;
					VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					for (auto& layoutBinding : layoutBindings) 
						if (layoutBinding.descriptorType == descriptorType && layoutBinding.binding == binding)
						{
							add = false;
							layoutBinding.stageFlags |= shaderStage;
							break;
						}					

					if (add) 
					{
						auto& layoutBinding = layoutBindings.emplace_back();

						const auto& type = compiler.get_type(resource.type_id);

						uint32_t descriptorCount = 1;
						//if (type.array[0] > 0) descriptorCount = type.array[0];

						layoutBinding.descriptorType = descriptorType;
						layoutBinding.descriptorCount = descriptorCount;
						layoutBinding.stageFlags = shaderStage;
						layoutBinding.binding = binding;
						layoutBinding.pImmutableSamplers = nullptr;
					}
				}


				for (auto& resource : resources.storage_images) 
				{
					auto& layoutBinding = layoutBindings.emplace_back();

					const auto& type = compiler.get_type(resource.type_id);

					uint32_t descriptorCount = 1;
					//if (type.array[0] > 0) descriptorCount = type.array[0];

					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					layoutBinding.descriptorCount = descriptorCount;
					layoutBinding.stageFlags = shaderStage;
					layoutBinding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					layoutBinding.pImmutableSamplers = nullptr;
				}

				for (auto& resource : resources.sampled_images) 
				{
					auto& layoutBinding = layoutBindings.emplace_back();

					const auto& type = compiler.get_type(resource.type_id);

					uint32_t descriptorCount = 1;
					//if (type.array[0] > 0) descriptorCount = type.array[0];

					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					layoutBinding.descriptorCount = descriptorCount;
					layoutBinding.stageFlags = shaderStage;
					layoutBinding.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					layoutBinding.pImmutableSamplers = nullptr;
				}

				if (createInfo.dynamicDescriptorCount)
					layoutBindings[layoutBindings.size() - 1].descriptorCount = createInfo.dynamicDescriptorCount;

				VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };

				layoutInfo.pBindings = layoutBindings.data();
				layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());

				VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };

				if (createInfo.bindingFlagCount > 0) 
				{
					binding_flags.bindingCount = createInfo.bindingFlagCount;
					binding_flags.pBindingFlags = createInfo.bindingFlags;

					layoutInfo.pNext = &binding_flags;
				}

				m_DescriptorLayout.Create(layoutInfo);

				VkPipelineLayoutCreateInfo info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

				info.setLayoutCount = 1;
				info.pSetLayouts = m_DescriptorLayout.GetPointer();

				VkPushConstantRange range;
				range.size = 0;
				range.offset = 0;
				range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				if (resources.push_constant_buffers.size() > 0) 
				{
					auto& baseType = compiler.get_type(resources.push_constant_buffers[0].base_type_id);
					range.size = (uint32_t)compiler.get_declared_struct_size(baseType);

					info.pPushConstantRanges = &range;
					info.pushConstantRangeCount = 1;
				}

				vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &info, VulkanContext::GetAllocator(), &m_PipelineLayout);
			}

			//finally build the pipeline
			VkComputePipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };

			
			VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

			stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			stage.module = shaderModule;
			stage.pName = "main";
			pipelineCreateInfo.stage = stage;			
			pipelineCreateInfo.layout = m_PipelineLayout;

			vkCreateComputePipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, VulkanContext::GetAllocator(), &m_Pipeline);

			vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModule, nullptr);
		}

		void Create(const std::string& path) 
		{
			ComputeShaderCreateInfo info;
			info.path = path;
			Create(info);
		}

		void Create(const std::string& path, const std::string& infoPath) 
		{
			ComputeShaderCreateInfo createInfo;
			createInfo.path = path;
			//createInfo.infoPath = infoPath;
			Create(createInfo);
		}

		void Bind(CommandBuffer cmd) { vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline); }

		void PushConstants(CommandBuffer cmd, uint32_t size, const void* data, uint32_t offset = 0) 
		{ cmd.PushConstants(m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, size, data, offset);	}

		void Destroy() 
		{
			vkDestroyPipeline(VulkanContext::GetLogicalDevice(), m_Pipeline, VulkanContext::GetAllocator());
			m_Pipeline = VK_NULL_HANDLE;

			vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), m_PipelineLayout, VulkanContext::GetAllocator());
			m_PipelineLayout = VK_NULL_HANDLE;

			m_DescriptorLayout.Destroy();
		}
	};
}