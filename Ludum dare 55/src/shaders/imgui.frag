#pragma shader_stage(fragment)

layout(location = 0) out vec4 fColor;

layout(binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec4 Color;
layout(location = 1) in vec2 uv;

void main()
{
    fColor = Color * texture(u_Texture, uv);
}