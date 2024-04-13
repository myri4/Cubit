#pragma once

#include <wc/Framebuffer.h>
#include <wc/Math/Camera.h>
#include <wc/Shader.h>
#include <wc/vk/Descriptors.h>

#include "RenderData.h"
#include <imgui/imgui_impl_vulkan.h>

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
		Sampler m_ScreenSampler;
		OrthographicCamera* camera = nullptr;
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

			m_ImageID = MakeImGuiDescriptor(m_ImageID, { m_ScreenSampler, GetRenderAttachment().view, VK_IMAGE_LAYOUT_GENERAL });

			//UploadContext::immediate_submit([&](VkCommandBuffer cmd) {
			//	GetRenderAttachment().image.setLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
			//	});

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
			SamplerCreateInfo sampler;

			sampler.magFilter = Filter::LINEAR;
			sampler.minFilter = Filter::LINEAR;
			sampler.mipmapMode = SamplerMipmapMode::LINEAR;
			sampler.addressModeU = SamplerAddressMode::REPEAT;
			sampler.addressModeV = SamplerAddressMode::REPEAT;
			sampler.addressModeW = SamplerAddressMode::REPEAT;
			sampler.minLod = 0.f;
			sampler.maxLod = 1.f;

			m_ScreenSampler.Create(sampler);
			camera = &cameraptr;
		}

		void Resize(glm::vec2 newSize, RenderData& renderData)
		{
			DestroyScreen();
			CreateScreen(newSize, renderData);
		}

		void Flush(RenderData& renderData)
		{
			//if (!m_IndexCount && !m_LineVertexCount) return;

			CommandBuffer& cmd = SyncContext::MainCommandBuffer;

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

			glm::mat4 proj = camera->GetViewProjectionMatrix();

			if (renderData.GetIndexCount())
			{
				renderData.UploadVertexData();
				m_Shader.Bind(cmd);

				cmd.PushConstants(m_Shader.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(proj), &proj);
				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, 0, m_Shader.GetPipelineLayout(), m_DescriptorSet);
				cmd.BindIndexBuffer(renderData.GetIndexBuffer());
				cmd.DrawIndexed(renderData.GetIndexCount());
			}

			if (renderData.GetLineVertexCount())
			{
				renderData.UploadLineVertexData();

				m_LineShader.Bind(cmd);
				cmd.PushConstants(m_Shader.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(proj), &proj);
				cmd.BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, 0, m_LineShader.GetPipelineLayout(), m_LineDescriptorSet);
				cmd.Draw(renderData.GetLineVertexCount());
			}


			cmd.EndRenderPass();
			cmd.End();


			VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

			submit.commandBufferCount = 1;
			submit.pCommandBuffers = cmd.GetPointer();

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			submit.pWaitDstStageMask = &waitStage;

			VulkanContext::graphicsQueue.Submit(submit, SyncContext::RenderFence);
			SyncContext::RenderFence.Wait();
			SyncContext::RenderFence.Reset();
			cmd.Reset();
		}

		void DestroyScreen()
		{
			m_Framebuffer.Destroy();

			m_Shader.Destroy();
			m_LineShader.Destroy();
		}

		void Deinit()
		{
			m_ScreenSampler.Destroy();

			DestroyScreen();
		}
	};
}