#ifndef MATH_GLSL
#define MATH_GLSL

#define saturate(x)        clamp(x, 0.f, 1.f)
#define atan2(x, y)        atan(y, x)

//------------------------------------------------------------------------------
// Scalar operations
//------------------------------------------------------------------------------

/**
 * Computes x^5 using only multiply operations.
 *
 * @public-api
 */
float pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

/**
 * Computes x^2 as a single multiplication.
 *
 * @public-api
 */
float sq(float x) { return x * x; }

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

/**
 * Returns the maximum component of the specified vector.
 *
 * @public-api
 */
float max3(const vec3 v) { return max(v.x, max(v.y, v.z)); }

float vmax(const vec2 v) { return max(v.x, v.y); }

float vmax(const vec3 v) { return max(v.x, max(v.y, v.z)); }

float vmax(const vec4 v) { return max(max(v.x, v.y), max(v.y, v.z)); }

float min3(const vec3 v) { return min(v.x, min(v.y, v.z)); }

float vmin(const vec2 v) { return min(v.x, v.y); }

float vmin(const vec3 v) { return min(v.x, min(v.y, v.z)); }

float vmin(const vec4 v) { return min(min(v.x, v.y), min(v.y, v.z)); }

float luminance(const vec3 linear) { return dot(linear, vec3(0.2126, 0.7152, 0.0722)); }

vec3 ycbcrToRgb(float luminance, vec2 cbcr) 
{
    // Taken from https://developer.apple.com/documentation/arkit/arframe/2867984-capturedimage
    const mat4 ycbcrToRgbTransform = mat4(
         1.0000,  1.0000,  1.0000,  0.0000,
         0.0000, -0.3441,  1.7720,  0.0000,
         1.4020, -0.7141,  0.0000,  0.0000,
        -0.7010,  0.5291, -0.8860,  1.0000
    );
    return (ycbcrToRgbTransform * vec4(luminance, cbcr, 1.0)).rgb;
}

float rtLerp(const in vec3 uvw, const in float a, const in float b, const in float c) { return uvw.x * a + uvw.y * b + uvw.z * c; }
vec2 rtLerp(const in vec3 uvw, const in vec2 a, const in vec2 b, const in vec2 c) { return uvw.x * a + uvw.y * b + uvw.z * c; }
vec3 rtLerp(const in vec3 uvw, const in vec3 a, const in vec3 b, const in vec3 c) { return uvw.x * a + uvw.y * b + uvw.z * c; }
vec4 rtLerp(const in vec3 uvw, const in vec4 a, const in vec4 b, const in vec4 c) { return uvw.x * a + uvw.y * b + uvw.z * c; }

uint convertColor(const in vec4 color) 
{ // Remember! Convert from 0-1 to 0-255!
	int r = int(color.r);
	int g = int(color.g);
	int b = int(color.b);
	int a = int(color.a);
	return a << 24 | b << 16 | g << 8 | r;
}

vec4 decompress(const in uint num) 
{ // Remember! Convert from 0-255 to 0-1!
    vec4 Output;
    Output.r = float((num & uint(0x000000ff)));
    Output.g = float((num & uint(0x0000ff00)) >> 8);
    Output.b = float((num & uint(0x00ff0000)) >> 16);
    Output.a = float((num & uint(0xff000000)) >> 24);
    return Output;
}

// NOTE: x and z are swapped here cuz idk... gpus...
int to1D(const in ivec3 pos, const in uint size) { return int((pos.x * size * size) + (pos.y * size) + pos.z); }
	
ivec3 to3D(int i, const in ivec3 size) 
{
	int z = i / (size.x * size.y);
	i -= (z * size.x * size.y);
	int y = i / size.x;
	int x = i % size.x;
	return ivec3(x, y, z);
}

#endif