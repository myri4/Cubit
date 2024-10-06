#pragma once

#include <wc/Framebuffer.h>
#include <wc/Math/Camera.h>
#include <wc/Shader.h>
#include <wc/vk/Descriptors.h>

#include "BloomEffect.h"

#include "RenderData.h"
#include <imgui/imgui_impl_vulkan.h>

#include "Font.h"

namespace wc
{
	class Renderer2D
	{
		glm::vec2 m_TileSize = glm::vec2(16.f, 9.f); // @TODO: maybe rename?

		glm::vec2 m_RenderSize;
		float m_AspectRatio = 16.f / 9.f;

		// Rendering
		Framebuffer m_Framebuffer;
		Shader m_Shader;
		DescriptorSet m_DescriptorSet;

		Shader m_LineShader;
		DescriptorSet m_LineDescriptorSet;

		VkDescriptorSet m_ImageID = VK_NULL_HANDLE;
		OrthographicCamera* camera = nullptr;

		// Post processing
		ComputeShader m_BackgroundShader;
		DescriptorSet m_BackgroundSet;

		ComputeShader m_CompositeShader;
		DescriptorSet m_CompositeSet;

		ComputeShader m_ChromaShader;
		DescriptorSet m_ChromaSet;

		ComputeShader m_CRTShader;
		DescriptorSet m_CRTSet;

		float time = 0.f;

		Image m_FinalImage[3];
		ImageView m_FinalImageView[3];

		CommandBuffer m_Cmd[FRAME_OVERLAP];
		CommandBuffer m_ComputeCmd[FRAME_OVERLAP];

		// Radiance cascades
		ComputeShader m_RCJFAShader;
		ComputeShader m_RCGIShader;

		Image m_RCBufferImage[2];
		ImageView m_RCBufferImageView[2];

		Image m_RCDistImage;
		ImageView m_RCDistImageView;

		uint32_t m_RCCascadeCount = 0;
		uint32_t m_RCBaseRayCount = 16;
		uint32_t m_RCJFAPasses = 0;
		std::vector<DescriptorSet> m_RCJFASets;
		std::vector<DescriptorSet> m_RCGISets;
		DescriptorSet m_RCSeedDescriptor;
		DescriptorSet m_RCGIDescriptor;


	public:
		uint32_t BackgroundTexture = 0;

		struct ChromaticAberrationSettings
		{
			uint32_t SampleCount = 50;
			float Blur = 0.25f;
			float Falloff = 7.f;
		}ChromaSettings;

	public:
		auto GetRenderImageID() { return m_ImageID; }
		auto GetRenderImage() { return m_Framebuffer.attachments[0].image; }
		auto GetRenderAttachment() { return m_Framebuffer.attachments[0]; }

		auto GetRenderSize() const { return m_RenderSize; }
		auto GetAspectRatio() const { return m_AspectRatio; }

		auto GetHalfSize() const { return m_TileSize * camera->Zoom; }
		auto GetHalfSize(glm::vec2 size) const { return size * camera->Zoom; }

		auto ScreenToWorld(glm::vec2 coords) const
		{
			float camX = ((2.f * coords.x / m_RenderSize.x) - 1.f);
			float camY = (1.f - (2.f * coords.y / m_RenderSize.y));
			return glm::vec2(camX, camY) * GetHalfSize();
		}

		auto WorldToScreen(glm::vec2 worldCoords) const
		{
			glm::vec2 relativeCoords = worldCoords / GetHalfSize();

			float screenX = ((relativeCoords.x + 1.f) / 2.f) * m_RenderSize.x;
			float screenY = ((1.f - relativeCoords.y) / 2.f) * m_RenderSize.y;

			return glm::vec2(screenX, screenY);
		}

		// Initializes size-independent data
		void Init(OrthographicCamera& cameraptr)
		{			
			camera = &cameraptr;

			InitBloom();
			
			m_BackgroundShader.Create("assets/shaders/background.comp");
			descriptorAllocator.allocate(m_BackgroundSet, m_BackgroundShader.GetDescriptorLayout());

			m_CompositeShader.Create("assets/shaders/composite.comp");
			descriptorAllocator.allocate(m_CompositeSet, m_CompositeShader.GetDescriptorLayout());

			m_ChromaShader.Create("assets/shaders/chromaticAberration.comp");
			descriptorAllocator.allocate(m_ChromaSet, m_ChromaShader.GetDescriptorLayout());

			m_CRTShader.Create("assets/shaders/crt.comp");
			descriptorAllocator.allocate(m_CRTSet, m_CRTShader.GetDescriptorLayout());

			for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
			{
				SyncContext::CommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_Cmd[i]);
				SyncContext::ComputeCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_ComputeCmd[i]);
			}

			m_RCJFAShader.Create("assets/shaders/RCJFA.comp");
			descriptorAllocator.allocate(m_RCSeedDescriptor, m_RCJFAShader.GetDescriptorLayout());

			m_RCGIShader.Create("assets/shaders/RCGI.comp");
			descriptorAllocator.allocate(m_RCGIDescriptor, m_RCGIShader.GetDescriptorLayout());
		}

		void CreateScreen(glm::vec2 size, RenderData& renderData)
		{
			m_RenderSize = size;
			m_AspectRatio = m_RenderSize.x / m_RenderSize.y;

			camera->Update(GetHalfSize());
			AttachmentCreateInfo attachmentInfo = {};
			attachmentInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attachmentInfo.width = (uint32_t)m_RenderSize.x;
			attachmentInfo.height = (uint32_t)m_RenderSize.y;
			attachmentInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT /*| VK_IMAGE_USAGE_STORAGE_BIT*/;
			m_Framebuffer.AddAttachment(attachmentInfo);

			m_Framebuffer.Create(m_RenderSize);
			m_Framebuffer.attachments[0].image.SetName("m_Framebuffer.attachments[0]");

			for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
			{
				ImageCreateInfo imageInfo;
				imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				imageInfo.width = m_RenderSize.x;
				imageInfo.height = m_RenderSize.y;
				imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

				m_FinalImage[i].Create(imageInfo);
				m_FinalImage[i].SetName(std::format("Renderer2D::FinalImage[{}]", i));

				m_FinalImageView[i].Create(m_FinalImage[i]);
				m_FinalImageView[i].SetName(std::format("Renderer2D::FinalImageView[{}]", i));
			}

			for (int i = 0; i < ARRAYSIZE(m_RCBufferImage); i++)
			{
				ImageCreateInfo imageInfo;
				imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				imageInfo.width = m_RenderSize.x;
				imageInfo.height = m_RenderSize.y;
				imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
				m_RCBufferImage[i].Create(imageInfo);
				m_RCBufferImage[i].SetName(std::format("Renderer2D::m_RCBufferImage[{}]", i));
				m_RCBufferImageView[i].Create(m_RCBufferImage[i]);
			}

			{ // @TODO: consider making this R32 texture instead
				ImageCreateInfo imageInfo;
				imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				imageInfo.width = m_RenderSize.x;
				imageInfo.height = m_RenderSize.y;
				imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
				m_RCDistImage.Create(imageInfo);

				m_RCDistImageView.Create(m_RCDistImage);
			}

			{
				TextureCreateInfo textureInfo;
				textureInfo.width = m_RenderSize.x;
				textureInfo.height = m_RenderSize.y;
				textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				if (BackgroundTexture == 0) BackgroundTexture = renderData.AllocateTexture(textureInfo);
			}

			SyncContext::immediate_submit([&](VkCommandBuffer cmd) {
				for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
					m_FinalImage[i].setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
				for (int i = 0; i < ARRAYSIZE(m_RCBufferImage); i++)
					m_RCBufferImage[i].setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

				m_RCDistImage.setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

				if (BackgroundTexture == 0) renderData.Textures[BackgroundTexture].GetImage().setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
			});

			CreateBloomImages(m_RenderSize, m_Framebuffer.attachments[0].image.GetMipLevelCount());

			// For now we are using the same sampler for sampling the screen and the bloom images but maybe it should be separated
			SamplerCreateInfo sampler;
			sampler.magFilter = Filter::LINEAR;
			sampler.minFilter = Filter::LINEAR;
			sampler.mipmapMode = SamplerMipmapMode::LINEAR;
			sampler.addressModeU = SamplerAddressMode::CLAMP_TO_EDGE;
			sampler.addressModeV = SamplerAddressMode::CLAMP_TO_EDGE;
			sampler.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
			sampler.minLod = 0.f;
			sampler.maxLod = float(m_BloomMipLevels);

			m_ScreenSampler.Create(sampler);
			{
				ShaderCreateInfo createInfo;
				createInfo.vertexShader = "assets/shaders/Renderer2D.vert";
				createInfo.fragmentShader = "assets/shaders/Renderer2D.frag";
				createInfo.renderSize = m_RenderSize;
				createInfo.renderPass = m_Framebuffer.renderPass;
				createInfo.blending = true;
				createInfo.depthTest = false;

				VkDescriptorBindingFlags flags[2];
				memset(flags, 0, sizeof(VkDescriptorBindingFlags) * (std::size(flags) - 1));
				flags[std::size(flags) - 1] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

				uint32_t count = (uint32_t)renderData.Textures.size();

				VkDescriptorSetVariableDescriptorCountAllocateInfo set_counts = {};
				set_counts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
				set_counts.descriptorSetCount = 1;
				set_counts.pDescriptorCounts = &count;

				createInfo.bindingFlags = flags;
				createInfo.bindingFlagCount = (uint32_t)std::size(flags);

				createInfo.dynamicDescriptorCount = count;

				m_Shader.Create(createInfo);

				descriptorAllocator.allocate(m_DescriptorSet, m_Shader.GetDescriptorLayout(), &set_counts, set_counts.descriptorSetCount);
			}	
			CreateBloom(m_Framebuffer.attachments[0].view);
			{
				DescriptorWriter writer;
				writer.dstSet = m_CompositeSet;
				writer.write_image(0, { m_ScreenSampler, m_FinalImageView[0], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					.write_image(1, { m_ScreenSampler, m_Framebuffer.attachments[0].view, VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.write_image(2, { m_ScreenSampler, m_BloomBuffers[2].imageViews[0], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.Update();
			}
			{
				DescriptorWriter writer;
				writer.dstSet = m_ChromaSet;
				writer.write_image(0, { m_ScreenSampler, m_FinalImageView[1], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					.write_image(1, { m_ScreenSampler, m_FinalImageView[0], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.Update();
			}
			{
				DescriptorWriter writer;
				writer.dstSet = m_CRTSet;
				writer.write_image(0, { m_ScreenSampler, m_FinalImageView[2], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					.write_image(1, { m_ScreenSampler, m_FinalImageView[1], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.Update();
			}
			{
				auto& texture = renderData.Textures[BackgroundTexture];
				DescriptorWriter writer;
				writer.dstSet = m_BackgroundSet;
				writer.write_image(0, { texture.GetSampler(), texture.GetView(), VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					.Update();
			}
			// Radiance cascades
			{
				DescriptorWriter writer;
				writer.dstSet = m_RCSeedDescriptor;
				writer.write_image(0, { m_ScreenSampler, m_RCBufferImageView[0], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				writer.write_image(1, { m_ScreenSampler, m_Framebuffer.attachments[0].view, VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.Update();
			}

			auto diagonal = glm::sqrt(m_RenderSize.x * m_RenderSize.x + m_RenderSize.y * m_RenderSize.y);

			// Our calculation for number of cascades
			m_RCCascadeCount = glm::ceil(glm::log(diagonal) / glm::log(m_RCBaseRayCount)) + 1;
			
			m_RCJFAPasses = glm::ceil(glm::log2(glm::max(m_RenderSize.x, m_RenderSize.y)));
			//m_RCJFASets.reserve(m_RCJFAPasses); // @NOTE: See bloom descriptor set vector clearing
			for (int i = 0; i < m_RCJFAPasses; i++)
			{
				DescriptorSet descriptor = m_RCJFAShader.AllocateDescriptorSet();
				DescriptorWriter writer;
				writer.dstSet = descriptor;
				writer.write_image(0, { m_ScreenSampler, m_RCBufferImageView[(i + 1) % 2], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				writer.write_image(1, { m_ScreenSampler, m_RCBufferImageView[(i) % 2], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.Update();
				m_RCJFASets.push_back(descriptor);
			}
			{
				DescriptorSet descriptor = m_RCJFAShader.AllocateDescriptorSet();
				DescriptorWriter writer;
				writer.dstSet = descriptor;
				writer.write_image(0, { m_ScreenSampler, m_RCDistImageView, VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				writer.write_image(1, { m_ScreenSampler, m_RCBufferImageView[(m_RCJFAPasses) % 2], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.Update();
				m_RCJFASets.push_back(descriptor);
			}
			for (int i = m_RCCascadeCount - 1; i >= 0; i--)
			{
				DescriptorSet descriptor = m_RCGIShader.AllocateDescriptorSet();
				DescriptorWriter writer;
				writer.dstSet = descriptor;
				writer.write_image(0, { m_ScreenSampler, m_RCBufferImageView[(i) % 2], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				writer.write_image(1, { m_ScreenSampler, m_RCDistImageView, VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.write_image(2, { m_ScreenSampler, m_Framebuffer.attachments[0].view, VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.write_image(3, { m_ScreenSampler, m_RCBufferImageView[(i + 1) % 2], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.Update();
				m_RCGISets.push_back(descriptor);
			}
			{
				descriptorAllocator.allocate(m_RCGIDescriptor, m_RCGIShader.GetDescriptorLayout());
				int i = m_RCJFAPasses;
				DescriptorWriter writer;
				writer.dstSet = m_RCGIDescriptor;
				writer.write_image(0, { m_ScreenSampler, m_RCBufferImageView[(m_RCJFAPasses) % 2], VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
				writer.write_image(1, { m_ScreenSampler, m_RCDistImageView, VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.write_image(2, { m_ScreenSampler, m_Framebuffer.attachments[0].view, VK_IMAGE_LAYOUT_GENERAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.Update();
			}

			{
				ShaderCreateInfo createInfo;
				createInfo.vertexShader = "assets/shaders/Line.vert";
				createInfo.fragmentShader = "assets/shaders/Line.frag";
				createInfo.renderSize = m_RenderSize;
				createInfo.renderPass = m_Framebuffer.renderPass;
				createInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				createInfo.blending = true;
				createInfo.depthTest = false;

				m_LineShader.Create(createInfo);

				descriptorAllocator.allocate(m_LineDescriptorSet, m_LineShader.GetDescriptorLayout());


				DescriptorWriter writer;
				writer.dstSet = m_LineDescriptorSet;
				writer.write_buffer(0, renderData.GetLineVertexBuffer().GetDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

				writer.Update();
			}

			m_ImageID = MakeImGuiDescriptor(m_ImageID, { m_ScreenSampler, m_RCBufferImageView[1], VK_IMAGE_LAYOUT_GENERAL });
			{
				DescriptorWriter writer;
				writer.dstSet = m_DescriptorSet;
				writer.write_buffer(0, renderData.GetVertexBuffer().GetDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

				std::vector<VkDescriptorImageInfo> infos;
				for (auto& image : renderData.Textures)
				{
					VkDescriptorImageInfo imageInfo;
					imageInfo.sampler = image.GetSampler();
					imageInfo.imageView = image.GetView();
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					infos.push_back(imageInfo);
				}

				writer.write_images(1, infos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

				writer.Update();
			}
		}

		void Resize(glm::vec2 newSize, RenderData& renderData)
		{
			DestroyScreen();
			CreateScreen(newSize, renderData);
		}

		void DestroyScreen()
		{
			m_Framebuffer.Destroy();

			m_Shader.Destroy();
			m_LineShader.Destroy();

			m_ScreenSampler.Destroy();

			for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
			{
				m_FinalImage[i].Destroy();
				m_FinalImageView[i].Destroy();
			}

			for (int i = 0; i < ARRAYSIZE(m_RCBufferImage); i++)
			{
				m_RCBufferImage[i].Destroy();
				m_RCBufferImageView[i].Destroy();
			}

			m_RCDistImage.Destroy();
			m_RCDistImageView.Destroy();

			m_RCJFASets.clear();

			DestroyBloomImages();
		}

		void Deinit()
		{
			DeinitBloom();
			m_BackgroundShader.Destroy();
			m_CompositeShader.Destroy();
			m_ChromaShader.Destroy();
			m_CRTShader.Destroy();
			m_RCJFAShader.Destroy();
			m_RCGIShader.Destroy();

			DestroyScreen();
		}

		void Flush(RenderData& renderData)
		{
			//if (!m_IndexCount && !m_LineVertexCount) return;

			time += Globals.deltaTime;
			if (Globals.settings.Background)
			{
				CommandBuffer& cmd = m_ComputeCmd[CURRENT_FRAME];
				cmd.Reset();
				cmd.Begin();

				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BackgroundShader.GetPipelineLayout(), m_BackgroundSet);
				m_BackgroundShader.Bind(cmd);
				struct {
					float time = 0.f;
					float zoom = 0.f;
					glm::vec2 cameraPos;
				} m_Data;
				m_Data.time = time;
				m_Data.zoom = camera->Zoom;
				m_Data.cameraPos = glm::vec2(camera->Position);
				m_BackgroundShader.PushConstants(cmd, m_Data);
				cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));

				cmd.End();

				SyncContext::Submit(cmd, SyncContext::GetComputeQueue());
			}

			{
				CommandBuffer& cmd = m_Cmd[CURRENT_FRAME];
				cmd.Reset();
				cmd.Begin();
				VkRenderPassBeginInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

				rpInfo.renderPass = m_Framebuffer.renderPass;
				rpInfo.framebuffer = m_Framebuffer.framebuffer;
				rpInfo.clearValueCount = 1;
				VkClearValue clearValue;
				clearValue.color = { 0.f, 0.f, 0.f, 0.f };
				rpInfo.pClearValues = &clearValue;

				rpInfo.renderArea.extent = { (uint32_t)m_RenderSize.x, (uint32_t)m_RenderSize.y };

				cmd.BeginRenderPass(rpInfo);

				if (renderData.GetIndexCount())
				{
					renderData.UploadVertexData();
					m_Shader.Bind(cmd);

					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, 0, m_Shader.GetPipelineLayout(), m_DescriptorSet);
					cmd.BindIndexBuffer(renderData.GetIndexBuffer());
					cmd.DrawIndexed(renderData.GetIndexCount());
				}

				if (renderData.GetLineVertexCount())
				{
					renderData.UploadLineVertexData();

					m_LineShader.Bind(cmd);
					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, 0, m_LineShader.GetPipelineLayout(), m_LineDescriptorSet);
					cmd.Draw(renderData.GetLineVertexCount());
				}


				cmd.EndRenderPass();
				cmd.End();

				SyncContext::Submit(cmd, SyncContext::GetGraphicsQueue());
			}

			{				
				CommandBuffer& cmd = m_ComputeCmd[CURRENT_FRAME];
				cmd.Reset();
				cmd.Begin();

				if (true)
				{
					{
						enum
						{
							MODE_SEED,
							MODE_JFA,
							MODE_SDF,
						};
						struct
						{
							glm::vec2 uOffset;
							uint32_t mode = MODE_SEED;
						}u_data;
						cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_RCJFAShader.GetPipelineLayout(), m_RCSeedDescriptor);
						m_RCJFAShader.PushConstants(cmd, u_data);

						m_RCJFAShader.Bind(cmd);
						cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));
						u_data.mode = MODE_JFA;
						for (int i = 0; i < m_RCJFAPasses; i++)
						{
							u_data.uOffset = glm::vec2(glm::pow(2, m_RCJFAPasses - i - 1));
							cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_RCJFAShader.GetPipelineLayout(), m_RCJFASets[i]);
							m_RCJFAShader.Bind(cmd);
							m_RCJFAShader.PushConstants(cmd, u_data);
							cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));
						}

						cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_RCJFAShader.GetPipelineLayout(), m_RCJFASets[m_RCJFAPasses]);
						u_data.mode = MODE_SDF;
						m_RCJFAShader.PushConstants(cmd, u_data);
						m_RCJFAShader.Bind(cmd);
						cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));
					}

					//m_RCGIShader.Bind(cmd);
					//cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_RCGIShader.GetPipelineLayout(), m_RCGIDescriptor);
					//cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));
					struct
					{
						float base;

						//float rayCount;
						float sqrtBase;
						//float oneOverRayCount;
						float angleStepSize;
						uint32_t firstLevel;
						float spacing;
						float intervalStart;
						float intervalLength;
						float upperSpacing;

						float cascadeIndex;
						float cascadeCount;
					}u_data;
					u_data.base = m_RCBaseRayCount;
					u_data.cascadeCount = m_RCCascadeCount;
					u_data.sqrtBase = glm::sqrt(u_data.base);

					for (int i = m_RCCascadeCount - 1; i >= 0; i--)
					{
						u_data.cascadeIndex = i;
						u_data.firstLevel = u_data.cascadeIndex == 0.f;

						float rayCount = glm::pow(u_data.base, u_data.cascadeIndex + 1.f);
						u_data.angleStepSize = glm::tau<float>() / rayCount;
						u_data.spacing = glm::pow(u_data.sqrtBase, u_data.cascadeIndex);
						u_data.upperSpacing = glm::pow(u_data.sqrtBase, u_data.cascadeIndex + 1.0);
						float modifierHack = u_data.base < 16.0 ? 1.0 : 4.0; // Hand-wavy rule that improved smoothing of other base ray counts
						float oneOverShortestSize = 1.f / glm::min(m_RenderSize.x, m_RenderSize.y);
						u_data.intervalStart = u_data.firstLevel == 1 ? 0.0 : (modifierHack * pow(u_data.base, u_data.cascadeIndex - 1.0)) * oneOverShortestSize;
						u_data.intervalLength = (modifierHack * pow(u_data.base, u_data.cascadeIndex)) * oneOverShortestSize; // Calculate which set of rays we care about

						m_RCGIShader.Bind(cmd);
						m_RCGIShader.PushConstants(cmd, u_data);
						cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_RCGIShader.GetPipelineLayout(), m_RCGISets[i]);
						cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));
					}
				}

				if (Globals.settings.Bloom)
				{
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BloomShader.GetPipeline());
					uint32_t counter = 0;

					enum class BloomMode
					{
						Prefilter,
						Downsample,
						UpsampleFirst,
						Upsample
					};

					struct
					{
						glm::vec4 Params = glm::vec4(1.f); // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
						float LOD = 0.f;
						int Mode = (int)BloomMode::Prefilter;
					} settings;

					settings.Params = glm::vec4(BloomThreshold, BloomThreshold - BloomKnee, BloomKnee * 2.f, 0.25f / BloomKnee);
					m_BloomShader.PushConstants(cmd, settings);
					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
					counter++;
					cmd.Dispatch(glm::ceil(glm::vec2(m_BloomBuffers[0].image.GetSize()) / glm::vec2(m_ComputeWorkGroupSize)));

					settings.Mode = (int)BloomMode::Downsample;
					for (uint32_t currentMip = 1; currentMip < m_BloomMipLevels; currentMip++)
					{
						glm::vec2 dispatchSize = glm::ceil((glm::vec2)m_BloomBuffers[0].image.GetMipSize(currentMip) / glm::vec2(m_ComputeWorkGroupSize));

						// Ping 
						settings.LOD = float(currentMip - 1);
						m_BloomShader.PushConstants(cmd, settings);

						cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
						counter++;
						cmd.Dispatch(dispatchSize);

						// Pong 
						settings.LOD = float(currentMip);
						m_BloomShader.PushConstants(cmd, settings);

						cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
						counter++;
						cmd.Dispatch(dispatchSize);
					}

					// First Upsample		
					settings.LOD = float(m_BloomMipLevels - 2);
					settings.Mode = (int)BloomMode::UpsampleFirst;
					m_BloomShader.PushConstants(cmd, settings);

					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
					counter++;

					cmd.Dispatch(glm::ceil((glm::vec2)m_BloomBuffers[2].image.GetMipSize(m_BloomMipLevels - 1) / glm::vec2(m_ComputeWorkGroupSize)));

					settings.Mode = (int)BloomMode::Upsample;
					for (int currentMip = m_BloomMipLevels - 2; currentMip >= 0; currentMip--)
					{
						settings.LOD = float(currentMip);
						m_BloomShader.PushConstants(cmd, settings);

						cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
						counter++;

						cmd.Dispatch(glm::ceil((glm::vec2)m_BloomBuffers[2].image.GetMipSize(currentMip) / glm::vec2(m_ComputeWorkGroupSize)));
					}
				}

				{
					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_CompositeShader.GetPipelineLayout(), m_CompositeSet);
					m_CompositeShader.Bind(cmd);
					struct
					{
						uint32_t Bloom;
					}m_Data;
					m_Data.Bloom = Globals.settings.Bloom;
					m_CompositeShader.PushConstants(cmd, m_Data);
					cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));
				}

				if (true)
				{
					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_ChromaShader.GetPipelineLayout(), m_ChromaSet);
					m_ChromaShader.Bind(cmd);
					m_ChromaShader.PushConstants(cmd, ChromaSettings);
					cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));
				}

				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_CRTShader.GetPipelineLayout(), m_CRTSet);
				m_CRTShader.Bind(cmd);
				struct {
					float time = 0.f;
					uint32_t CRT;
					uint32_t Vignete;
					float Brighness;
				} m_Data;
				m_Data.time = time;
				m_Data.CRT = Globals.settings.CRTEffect;
				m_Data.Vignete = Globals.settings.Vignete;
				m_Data.Brighness = Globals.settings.Brighness;

				m_CRTShader.PushConstants(cmd, m_Data);
				cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));

				cmd.End();

				SyncContext::Submit(cmd, SyncContext::GetComputeQueue());
			}
		}
	};
}