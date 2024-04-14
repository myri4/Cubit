#ifndef RANDOM_GLSL
#define RANDOM_GLSL
#include "constants.glsl"

uint RandomUint(inout uint randomState)
{
	randomState = randomState * 246049789 % 268435399;
	uint c = randomState & 0xE0000000 >> 29;
	randomState = ((((randomState ^ randomState >> c)) ^ (c << 32 - c)) * 104122896) ^ (c << 7);
	return randomState;
}

float RandomValue(inout uint seed) { return RandomUint(seed) * uintBitsToFloat(0x2f800004U); }

float RandomValueNormalDistribution(inout uint seed)
{
	float theta = 2.f * PI * RandomValue(seed);
	float rho = sqrt(-2.f * log(RandomValue(seed)));
	return rho * cos(theta);
}

vec2 RandomDirection(inout uint seed)
{
	float x = RandomValueNormalDistribution(seed);
	float y = RandomValueNormalDistribution(seed);
	return normalize(vec2(x, y));
}

vec2 RandomHemisphereDirection(vec2 normal, inout uint seed)
{
	vec2 dir = RandomDirection(seed);
	return dir * sign(dot(normal, dir));
}

#endif