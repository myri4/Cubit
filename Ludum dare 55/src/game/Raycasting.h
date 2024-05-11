#pragma once

#include <glm/glm.hpp>

namespace wc
{
	struct Ray
	{
		glm::vec2 Origin;
		glm::vec2 Direction;
		glm::vec2 InvDirection;

		Ray() = default;
		Ray(glm::vec2 orig, glm::vec2 dir)
		{
			Origin = orig;
			Direction = dir;
			InvDirection = 1.f / Direction;
		}
	};

	const float bias = 0.01f;

	struct HitInfo
	{
		bool Hit = false;
		bool Inside = false;
		float t = 0.f;
		glm::vec2 Point;
		glm::vec2 N;
		glm::vec2 uv;
		void* Entity = nullptr;

		HitInfo() = default;
		HitInfo(bool hit) { Hit = hit; }
	};

	float cross(glm::vec2 u, glm::vec2 v) { return u.x * v.y - u.y * v.x; }

	struct LineHitInfo
	{
		bool Hit = false;
		float Nearest = 0.f;
		glm::vec2 Normal;
		//glm::vec2 uv;

		LineHitInfo() = default;
		LineHitInfo(bool hit) { Hit = hit; }
		LineHitInfo(bool hit, float nearest, glm::vec2 normal)
		{
			Hit = hit;
			Nearest = nearest;
			Normal = normal;
		}
	};

	LineHitInfo LineIntersection(const Ray& ray, glm::vec2 a, glm::vec2 b)
	{
		glm::vec2 v3(-ray.Direction.y, ray.Direction.x);

		glm::vec2 aToO = ray.Origin - a;
		glm::vec2 aToB = b - a;

		float dot = glm::dot(aToB, v3);
		if (dot == 0.f) return false;

		float invDot = 1.f / dot;
		float t1 = cross(aToB, aToO) * invDot;
		float t2 = glm::dot(aToO, v3) * invDot;

		return { t1 >= 0.f && t2 >= 0.f && t2 <= 1.f, t1, glm::normalize(glm::vec2(aToB.y, -aToB.x)) };
	}

	glm::vec2 LineNormal(const Ray& ray, glm::vec2 a, glm::vec2 b)
	{
		glm::vec2 aToO = ray.Origin - a;
		glm::vec2 aToB = b - a;

		return glm::normalize(glm::vec2(aToB.y, -aToB.x));
	}

	HitInfo aabbIntersection(const Ray& ray, glm::vec2 minimum, glm::vec2 maximum)
	{
		//from: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
		bool xSign = ray.Direction.x >= 0.f;
		bool ySign = ray.Direction.y >= 0.f;

		float nearest = ((xSign ? minimum : maximum).x - ray.Origin.x) * ray.InvDirection.x;
		float nextNearest = ((xSign ? maximum : minimum).x - ray.Origin.x) * ray.InvDirection.x;
		float nearestY = ((ySign ? minimum : maximum).y - ray.Origin.y) * ray.InvDirection.y;
		float nextNearestY = ((ySign ? maximum : minimum).y - ray.Origin.y) * ray.InvDirection.y;

		if (nearest > nextNearestY || nearestY > nextNearest)
			return false;

		HitInfo hitInfo;
		nearest = glm::max(nearestY, nearest);
		nextNearest = glm::min(nextNearestY, nextNearest);

		hitInfo.Inside = nearest < 0.f && nextNearest > 0.f;
		if (nearest < 0.f)
			nearest = nextNearest;


		if (hitInfo.Inside) hitInfo.Point = ray.Origin;
		else hitInfo.Point = ray.Origin + ray.Direction * nearest;

		//from: https://blog.johnnovak.net/2016/10/22/the-nim-raytracer-project-part-4-calculating-box-normals/
		glm::vec2 c = 0.5f * (minimum + maximum);
		glm::vec2 p = hitInfo.Point - c;
		glm::vec2 d = 0.5f * (minimum - maximum);
		hitInfo.N = glm::normalize(glm::vec2(glm::ivec2(p / abs(d) * bias))) * (hitInfo.Inside ? 1.f : -1.f);

		hitInfo.uv = (hitInfo.Point - minimum) / (maximum - minimum);
		hitInfo.uv.y = 1.f - hitInfo.uv.y;
		hitInfo.Hit = nextNearest > 0.f;
		hitInfo.t = nearest;
		return hitInfo;
	}

	glm::vec2 RandomOnHemisphere(const glm::vec2& normal, const glm::vec2& dir)
	{
		return dir * glm::sign(dot(normal, dir));
	}

	/*
	bool inTriangle(glm::vec2 pt)
	{
		float AB = (pt.y - p1.y) * (p2.x - p1.x) - (pt.x - p1.x) * (p2.y - p1.y);
		float CA = (pt.y - p3.y) * (p1.x - p3.x) - (pt.x - p3.x) * (p1.y - p3.y);
		float BC = (pt.y - p2.y) * (p3.x - p2.x) - (pt.x - p2.x) * (p3.y - p2.y);

		return AB * BC > 0.f && BC * CA > 0.f;
	}
	*/
}