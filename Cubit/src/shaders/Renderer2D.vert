#pragma shader_stage(vertex)

struct Vertex 
{
	vec3 Position;
	uint TextureID;
	vec2 TexCoords;
	float Fade;
	float Thickness;

	vec4 Color;
};

layout (std430, binding = 0) readonly buffer VertexBuffer { Vertex vertices[]; };

layout(location = 0) out vec2 v_TexCoords;
layout(location = 1) out flat uint v_TexID;
layout(location = 2) out vec4 v_Color;
layout(location = 3) out float v_Fade;
layout(location = 4) out float v_Thickness;

void main() 
{
	Vertex vertex = vertices[gl_VertexIndex];

    v_TexCoords = vertex.TexCoords;
	v_TexID = vertex.TextureID;
	v_Color = vertex.Color;
	v_Fade = vertex.Fade;
	v_Thickness = vertex.Thickness;

    gl_Position = vec4(vertex.Position, 1.f);
}