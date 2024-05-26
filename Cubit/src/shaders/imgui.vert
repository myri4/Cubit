#pragma shader_stage(vertex)

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in uint aColor;

layout(push_constant) uniform PushConstant
{ 
    vec2 uScale; 
    vec2 uTranslate; 
};

layout(location = 0) out vec4 Color;
layout(location = 1) out vec2 uv;

vec4 decompress(const in uint num) 
{ // Remember! Convert from 0-255 to 0-1!
    vec4 Output;
    Output.r = float((num & uint(0x000000ff)));
    Output.g = float((num & uint(0x0000ff00)) >> 8);
    Output.b = float((num & uint(0x00ff0000)) >> 16);
    Output.a = float((num & uint(0xff000000)) >> 24);
    return Output;
}

void main()
{
    Color = decompress(aColor) / 255.f;
    uv = aUV;
    gl_Position = vec4(aPos * uScale + uTranslate, 0, 1);
}