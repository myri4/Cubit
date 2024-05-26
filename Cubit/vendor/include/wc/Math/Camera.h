#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <random>

namespace wc
{
	struct Camera 
	{
		glm::vec3 Position;
		glm::vec3 Front = glm::vec3(0.f, 0.f, -1.f);
		glm::vec3 Up = glm::vec3(0.f, 1.f, 0.f);

		float Yaw = 0.f;
		float Pitch = 0.f;
		float Roll = glm::radians(90.f);
		float FOV = glm::radians(90.f);

		glm::mat4 GetViewMatrix() const { return glm::lookAt(Position, Position + Front, Up); }

		void Update(float aspectRatio) 
		{
			float cosPitch = glm::cos(Pitch);
			Front.x = glm::cos(Yaw) * cosPitch;
			Front.y = glm::sin(Pitch);
			Front.z = glm::sin(Yaw) * cosPitch;
			Front = glm::normalize(Front);

			glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
			//up.x = glm::cos(glm::radians(Roll));
			//up.y = glm::sin(glm::radians(Roll));
			//up = normalize(up);

			float theta = glm::tan(FOV * 0.5f);
			float viewport_height = 2.f * theta;
			float viewport_width = aspectRatio * viewport_height;

			glm::vec3 Right = glm::normalize(glm::cross(Front, up));
			Up = glm::normalize(glm::cross(Right, Front));
		}
	};

	class OrthographicCamera
	{
	private:
		glm::mat4 m_ProjectionMatrix;

	public:
		glm::vec3 Position;
		float Rotation = 0.f;
		float Zoom = 1.f;

	public:

		void Shake(float shake, float maxOffset = 0.3f, float maxRotation = 10.f)
		{
			shake *= shake;

			static std::uniform_real_distribution<float> rd(-1.f, 1.f);
			static std::mt19937 gen;

			glm::vec2 offset = maxOffset * shake * glm::normalize(glm::vec2(rd(gen), rd(gen)));
			float angle = maxRotation * shake * rd(gen);

			Position += glm::vec3(offset, 0.f);
			Rotation += angle;
		}

		void SetProjection(float left, float right, float bottom, float top, float Near = -1.f, float Far = 1.f) 
		{
			m_ProjectionMatrix = glm::ortho(left, right, bottom, top, Near, Far);
		}

		void Update(glm::vec2 halfSize) { SetProjection(-halfSize.x, halfSize.x, halfSize.y, -halfSize.y); }

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		
		glm::mat4 GetViewMatrix() const 
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.f), Position) * glm::rotate(glm::mat4(1.f), glm::radians(Rotation), glm::vec3(0.f, 0.f, 1.f));

			return glm::inverse(transform);
		}

		glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * GetViewMatrix(); }	
	};

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale) 
	{
		// From glm::decompose in matrix_decompose.inl
		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3], Pdum3;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		rotation.y = asin(-Row[0][2]);
		if (cos(rotation.y) != 0)
		{
			rotation.x = atan2(Row[1][2], Row[2][2]);
			rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			rotation.x = atan2(-Row[2][0], Row[1][1]);
			rotation.z = 0;
		}


		return true;
	}
		
}