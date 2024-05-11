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
		ComputeShader m_CompositeShader;
		DescriptorSet m_CompositeSet;

		ComputeShader m_ChromaShader;
		DescriptorSet m_ChromaSet;

		ComputeShader m_CRTShader;
		DescriptorSet m_CRTSet;

		float time = 0.f;

		Image m_FinalImage[3];
		ImageView m_FinalImageView[3];

		Semaphore m_RtoPPSemaphore[FRAME_OVERLAP]; // Semaphore for synchronizing between main rendering and post proccesing

		CommandBuffer m_Cmd[FRAME_OVERLAP];
		CommandBuffer m_ComputeCmd[FRAME_OVERLAP];
	public:
		Semaphore RenderSemaphore[FRAME_OVERLAP]; // Semaphore for signaling the end of the frame rendering

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

		auto GetHalfSize() const { return m_RenderSize / (2.f * 64.f) * camera->Zoom; }
		auto GetHalfSize(glm::vec2 size) const { return size / (2.f * 64.f) * camera->Zoom; }

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

				VkImageViewCreateInfo imageView = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
				imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageView.format = imageInfo.format;
				imageView.subresourceRange = {};
				imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageView.subresourceRange.layerCount = 1;
				imageView.subresourceRange.levelCount = 1;
				imageView.image = m_FinalImage[i];
				m_FinalImageView[i].Create(imageView);
				m_FinalImageView[i].SetName(std::format("Renderer2D::FinalImageView[{}]", i));
			}

			SyncContext::immediate_submit([&](VkCommandBuffer cmd) {
				for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
					m_FinalImage[i].setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
				});

			CreateBloomImages(m_RenderSize, m_Framebuffer.attachments[0].image.GetMipLevelCount());

			// For now we are using the same sampler for sampling the screen and the bloom images but maybe it should be separeted
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
				writer.write_image(0, GetDescriptorData(m_ScreenSampler, m_FinalImageView[0], VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					.write_image(1, GetDescriptorData(m_ScreenSampler, m_Framebuffer.attachments[0].view, VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.write_image(2, GetDescriptorData(m_ScreenSampler, m_BloomBuffers[2].imageViews[0], VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.Update();
			}
			{
				DescriptorWriter writer;
				writer.dstSet = m_ChromaSet;
				writer.write_image(0, GetDescriptorData(m_ScreenSampler, m_FinalImageView[1], VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					.write_image(1, GetDescriptorData(m_ScreenSampler, m_FinalImageView[0], VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.Update();
			}
			{
				DescriptorWriter writer;
				writer.dstSet = m_CRTSet;
				writer.write_image(0, GetDescriptorData(m_ScreenSampler, m_FinalImageView[2], VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					.write_image(1, GetDescriptorData(m_ScreenSampler, m_FinalImageView[1], VK_IMAGE_LAYOUT_GENERAL), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.Update();
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

			m_ImageID = MakeImGuiDescriptor(m_ImageID, { m_ScreenSampler, m_FinalImageView[2], VK_IMAGE_LAYOUT_GENERAL });
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

		// Intitializes size-independant data
		void Init(OrthographicCamera& cameraptr)
		{			
			camera = &cameraptr;
			
			m_CompositeShader.Create("assets/shaders/composite.comp");
			descriptorAllocator.allocate(m_CompositeSet, m_CompositeShader.GetDescriptorLayout());

			m_ChromaShader.Create("assets/shaders/chromaticAberration.comp");
			descriptorAllocator.allocate(m_ChromaSet, m_ChromaShader.GetDescriptorLayout());

			m_CRTShader.Create("assets/shaders/crt.comp");
			descriptorAllocator.allocate(m_CRTSet, m_CRTShader.GetDescriptorLayout());

			for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
			{
				m_RtoPPSemaphore[i].Create();
				RenderSemaphore[i].Create();

				SyncContext::CommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_Cmd[i]);
				SyncContext::ComputeCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_ComputeCmd[i]);

				m_RtoPPSemaphore[i].SetName(std::format("Renderer2D::m_RtoPPSemaphore[{}]", i));
				RenderSemaphore[i].SetName(std::format("Renderer2D::RenderSemaphore[{}]", i));
			}
		}

		void Resize(glm::vec2 newSize, RenderData& renderData)
		{
			DestroyScreen();
			CreateScreen(newSize, renderData);
		}

		void Flush(RenderData& renderData)
		{
			//if (!m_IndexCount && !m_LineVertexCount) return;
			{
				CommandBuffer& cmd = m_Cmd[CURRENT_FRAME];
				cmd.Reset();
				cmd.Begin();
				VkRenderPassBeginInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

				rpInfo.renderPass = m_Framebuffer.renderPass;
				rpInfo.framebuffer = m_Framebuffer.framebuffer;
				rpInfo.clearValueCount = 1;
				VkClearValue clearValue;
				clearValue.color = { 0.f, 0.f, 0.f, 1.f };
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

				VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

				submit.commandBufferCount = 1;
				submit.pCommandBuffers = cmd.GetPointer();

				VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

				submit.pSignalSemaphores = m_RtoPPSemaphore[CURRENT_FRAME].GetPointer();
				submit.signalSemaphoreCount = 1;

				submit.pWaitDstStageMask = &waitStage;

				SyncContext::GetGraphicsQueue().Submit(submit);
			}

			{				
				CommandBuffer& cmd = m_ComputeCmd[CURRENT_FRAME];
				cmd.Reset();
				cmd.Begin();
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BloomShader.GetPipeline());
				uint32_t counter = 0;

				BloomBufferSettings settings;
				settings.Params = glm::vec4(BloomThreshold, BloomThreshold - BloomKnee, BloomKnee * 2.f, 0.25f / BloomKnee);
				m_BloomShader.PushConstants(cmd, sizeof(settings), &settings);
				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
				counter++;
				cmd.Dispatch(glm::ceil(glm::vec2(m_BloomBuffers[0].image.GetSize()) / glm::vec2(m_ComputeWorkGroupSize)));

				settings.Mode = (int)BloomMode::Downsample;
				for (uint32_t currentMip = 1; currentMip < m_BloomMipLevels; currentMip++)
				{
					glm::vec2 dispatchSize = glm::ceil((glm::vec2)m_BloomBuffers[0].image.GetMipSize(currentMip) / glm::vec2(m_ComputeWorkGroupSize));

					// Ping 
					settings.LOD = float(currentMip - 1);
					m_BloomShader.PushConstants(cmd, sizeof(settings), &settings);

					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
					counter++;
					cmd.Dispatch(dispatchSize);

					// Pong 
					settings.LOD = float(currentMip);
					m_BloomShader.PushConstants(cmd, sizeof(settings), &settings);

					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
					counter++;
					cmd.Dispatch(dispatchSize);
				}

				// First Upsample		
				settings.LOD = float(m_BloomMipLevels - 2);
				settings.Mode = (int)BloomMode::UpsampleFirst;
				m_BloomShader.PushConstants(cmd, sizeof(settings), &settings);

				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
				counter++;

				cmd.Dispatch(glm::ceil((glm::vec2)m_BloomBuffers[2].image.GetMipSize(m_BloomMipLevels - 1) / glm::vec2(m_ComputeWorkGroupSize)));

				settings.Mode = (int)BloomMode::Upsample;
				for (int currentMip = m_BloomMipLevels - 2; currentMip >= 0; currentMip--)
				{
					settings.LOD = float(currentMip);
					m_BloomShader.PushConstants(cmd, sizeof(settings), &settings);

					cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_BloomShader.GetPipelineLayout(), m_BloomSets[counter]);
					counter++;

					cmd.Dispatch(glm::ceil((glm::vec2)m_BloomBuffers[2].image.GetMipSize(currentMip) / glm::vec2(m_ComputeWorkGroupSize)));
				}

				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_CompositeShader.GetPipelineLayout(), m_CompositeSet);
				m_CompositeShader.Bind(cmd);
				cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));

				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_ChromaShader.GetPipelineLayout(), m_ChromaSet);
				m_ChromaShader.Bind(cmd);
				m_ChromaShader.PushConstants(cmd, sizeof(ChromaSettings), &ChromaSettings);
				cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));

				time += Globals.deltaTime;
				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, 0, m_CRTShader.GetPipelineLayout(), m_CRTSet);
				m_CRTShader.Bind(cmd);
				struct {
					float time = 0.f;
				} m_Data;
				m_Data.time = time;
				m_CRTShader.PushConstants(cmd, sizeof(m_Data), &m_Data);
				cmd.Dispatch(glm::ceil((glm::vec2)m_RenderSize / glm::vec2(m_ComputeWorkGroupSize)));

				cmd.End();

				VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

				submit.commandBufferCount = 1;
				submit.pCommandBuffers = cmd.GetPointer();

				VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

				submit.pWaitSemaphores = m_RtoPPSemaphore[CURRENT_FRAME].GetPointer();
				submit.waitSemaphoreCount = 1;

				submit.pSignalSemaphores = RenderSemaphore[CURRENT_FRAME].GetPointer();
				submit.signalSemaphoreCount = 1;

				submit.pWaitDstStageMask = &waitStage;

				SyncContext::GetComputeQueue().Submit(submit);
			}			
		}

		void DestroyScreen()
		{
			m_Framebuffer.Destroy();

			m_Shader.Destroy();
			m_LineShader.Destroy();

			for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
			{
				m_FinalImage[i].Destroy();
				m_FinalImageView[i].Destroy();
			}

			DestroyBloomImages();
		}

		void Deinit()
		{
			DeinitBloom();
			m_CompositeShader.Destroy();
			m_ChromaShader.Destroy();
			m_CRTShader.Destroy();

			for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
			{
				m_RtoPPSemaphore[i].Destroy();
				RenderSemaphore[i].Destroy();
			}

			DestroyScreen();
		}
	};
}