#pragma shader_stage(vertex)

struct Vertex {
	vec3 Position;
	uint _pad1;
	vec4 Color;
};

layout (std430, binding = 0) readonly buffer VertexBuffer { Vertex vertices[]; };

layout(location = 0) out vec4 v_Color;

layout (push_constant) uniform Camera
{
	mat4 u_ViewProjection;
};

void main() {
	Vertex vertex = vertices[gl_VertexIndex];
	v_Color = vertex.Color;    

    gl_Position = u_ViewProjection * vec4(vertex.Position, 1.f);
}