#include "constants.glsl"

struct Ray
{
    vec2 origin;
    vec2 direction;
    vec2 invDir;
};

struct MaterialData 
{
    vec4 color;
    uint textureID;
	//vec3 albedo;
	//uint albedoID;
	//vec3 emissive;
	//uint emissiveID;
};

struct Material
{
    vec4 albedo;
    vec3 emissive;
};

struct Circle
{
    vec2 Position;
    float Radius;
    uint _pad1;
    vec4 color;
};

struct AABB
{
    vec2 minimum;
    vec2 maximum;
    uint materialID;
    uint _pad[3];
};

struct Line
{
    vec2 start;
    vec2 end;
    vec4 color;
};

struct HitInfo
{
    bool hit;
    bool inside;
    float nearest;
    vec2 normal;
    vec2 p;
    uint materialID;
    vec2 uv;

    Material material;
};

const Material emptyMaterial = Material(vec4(0.f), vec3(0.f));
const HitInfo miss = HitInfo(false, false, -1000.f, vec2(0.f), vec2(0.f), 0, vec2(0.f), emptyMaterial);

float cross(vec2 u, vec2 v) { return u.x * v.y - u.y * v.x; }

struct LineHitInfo
{
    bool hit;
    float nearest;
    vec2 normal;
    vec4 color;
    //vec2 uv;
};

LineHitInfo LineIntersection(Ray ray, Line line)
{
	vec2 v1 = ray.origin - line.start;
	vec2 v2 = line.end - line.start;
	vec2 v3 = vec2(-ray.direction.y, ray.direction.x);

	float d = dot(v2, v3);
	float invDot = 1.f / d;
	float t1 = cross(v2, v1) * invDot;
	float t2 = dot(v1, v3) * invDot;

	return LineHitInfo(t1 >= 0.f && (t2 >= 0.f && t2 <= 1.f), t1, normalize(vec2(-v2.y, v2.x)), line.color);
}

HitInfo circleIntersection(Ray ray, Circle circle)
{
    float radius2 = circle.Radius * circle.Radius;
    vec2 L = circle.Position - ray.origin; 
    float tca = dot(L, ray.direction); 
    float d2 = dot(L, L) - tca * tca; 
    if (d2 > radius2) return miss;
    float thc = sqrt(radius2 - d2);
    float t0 = tca - thc;
    float t1 = tca + thc;
    float nearest = min(t0, t1);
    float nextNearest = max(t0, t1);
    
    bool inside = nearest < 0.f && nextNearest > 0.f;
    if(nearest < 0.f) 
        nearest = nextNearest;
    
    vec2 hit = ray.origin + ray.direction * nearest;
    vec2 normal = (circle.Position - hit) / circle.Radius;
    return HitInfo(nextNearest > 0.f, inside, nearest, inside ? -normal : normal, hit, 0, vec2(0.f), Material(circle.color, vec3(0.f)));
}

HitInfo aabbIntersection(Ray ray, AABB aabb)
{
    //from: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
    bool xSign = ray.direction.x >= 0.f;
    bool ySign = ray.direction.y >= 0.f;
    
    float nearest =      ((xSign ? aabb.minimum : aabb.maximum).x - ray.origin.x) * ray.invDir.x; 
    float nextNearest =  ((xSign ? aabb.maximum : aabb.minimum).x - ray.origin.x) * ray.invDir.x; 
    float nearestY =     ((ySign ? aabb.minimum : aabb.maximum).y - ray.origin.y) * ray.invDir.y; 
    float nextNearestY = ((ySign ? aabb.maximum : aabb.minimum).y - ray.origin.y) * ray.invDir.y; 
 
    if (nearest > nextNearestY || nearestY > nextNearest) 
        return miss;
    
    nearest = max(nearestY, nearest);
    nextNearest = min(nextNearestY, nextNearest);
    
    bool inside = nearest < .0 && nextNearest > .0;
    if(nearest < .0) 
        nearest = nextNearest;
    
    vec2 hit;
    if (inside) hit = ray.origin;
    else hit = ray.origin + ray.direction * nearest;
    
    //from: https://blog.johnnovak.net/2016/10/22/the-nim-raytracer-project-part-4-calculating-box-normals/
    vec2 c = 0.5f * (aabb.minimum + aabb.maximum);
    vec2 p = hit - c;
    vec2 d = 0.5f * (aabb.minimum - aabb.maximum);
    vec2 normal = normalize(vec2(ivec2( p / abs(d) * bias)));

    vec2 uv = (hit - aabb.minimum) / (aabb.maximum - aabb.minimum);
    uv.y = 1.f - uv.y;
    
    return HitInfo(nextNearest > .0, inside, nearest, inside ? normal : -normal, hit, aabb.materialID, uv, emptyMaterial); 
}