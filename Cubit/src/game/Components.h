#pragma once
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <wc/Utils/Time.h>
#include <wc/Utils/YAML.h>

namespace wc
{
	struct TransformComponent
	{
		glm::vec2 Position;
		glm::vec2 Size = glm::vec2(0.5f);
		float Rotation = 0.f;

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec2& translation) { Position = translation; }

		glm::mat4 GetTransform() const
		{
			auto Translation = glm::vec3(Position, 0.f);
			auto Scale = glm::vec3(Size, 1.f);

			return glm::translate(glm::mat4(1.f), Translation)
				* glm::rotate(glm::mat4(1.f), Rotation, { 0.f, 0.f, 1.f })
				* glm::scale(glm::mat4(1.f), Scale);
		}
	};

	//enum class BodyType { Static = 0, Dynamic, Kinematic };
	struct Rigidbody2DComponent
	{
		b2Body* Body = nullptr;

		uint32_t UpContacts = 0;
		uint32_t DownContacts = 0;
		uint32_t LeftContacts = 0;
		uint32_t RightContacts = 0;
		uint32_t Contacts = 0;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
	};
}