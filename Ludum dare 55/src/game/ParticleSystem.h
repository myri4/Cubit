#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

#include <vector>
#include <random>

#include "../Globals.h"
#include "../Rendering/RenderData.h"

namespace wc
{
	inline float RandomValue()
	{
		static std::uniform_real_distribution<float> distribution(0.f, 1.f);
		static std::mt19937 generator;
		return distribution(generator);
	}

	struct ParticleProps
	{
		glm::vec2 Position;
		glm::vec2 Velocity, VelocityVariation;
		glm::vec4 ColorBegin, ColorEnd;
		float SizeBegin, SizeEnd, SizeVariation;
		float LifeTime = 1.0f;
	};

	struct ParticleSystem
	{
		void Init()
		{
			m_ParticlePool.resize(1000);
		}

		void OnUpdate()
		{
			for (auto& particle : m_ParticlePool)
			{
				if (!particle.Active)
					continue;

				if (particle.LifeRemaining <= 0.0f)
				{
					particle.Active = false;
					continue;
				}

				particle.LifeRemaining -= Globals.deltaTime;
				particle.Position += particle.Velocity * (float)Globals.deltaTime;
				particle.Rotation += 0.01f * Globals.deltaTime;
			}
		}
		void OnRender(RenderData& renderData)
		{
			for (auto& particle : m_ParticlePool)
			{
				if (!particle.Active)
					continue;

				// Fade away particles
				float life = particle.LifeRemaining / particle.LifeTime;
				glm::vec4 color = glm::lerp(particle.ColorEnd, particle.ColorBegin, life);
				//color.a = color.a * life;

				float size = glm::lerp(particle.SizeEnd, particle.SizeBegin, life);

				// Render
				glm::mat4 transform = glm::translate(glm::mat4(1.f), { particle.Position.x, particle.Position.y, 0.0f })
					* glm::rotate(glm::mat4(1.f), particle.Rotation, { 0.f, 0.f, 1.f })
					* glm::scale(glm::mat4(1.f), { size, size, 1.0f });
				renderData.DrawQuad(transform, 0, color);
			}
		}

		void Emit(const ParticleProps& particleProps)
		{
			Particle& particle = m_ParticlePool[m_PoolIndex];
			particle.Active = true;
			particle.Position = particleProps.Position;
			particle.Rotation = RandomValue() * 2.f * glm::pi<float>();

			// Velocity
			particle.Velocity = particleProps.Velocity + particleProps.VelocityVariation * RandomValue();

			// Color
			particle.ColorBegin = particleProps.ColorBegin;
			particle.ColorEnd = particleProps.ColorEnd;

			particle.LifeTime = particleProps.LifeTime;
			particle.LifeRemaining = particleProps.LifeTime;
			particle.SizeBegin = particleProps.SizeBegin + particleProps.SizeVariation * RandomValue();
			particle.SizeEnd = particleProps.SizeEnd;

			m_PoolIndex = --m_PoolIndex % m_ParticlePool.size();
		}
	private:
		struct Particle
		{
			glm::vec2 Position;
			glm::vec2 Velocity;
			glm::vec4 ColorBegin, ColorEnd;
			float Rotation = 0.0f;
			float SizeBegin, SizeEnd;

			float LifeTime = 1.0f;
			float LifeRemaining = 0.0f;

			bool Active = false;
		};
		std::vector<Particle> m_ParticlePool;
		uint32_t m_PoolIndex = 999;
	};
}