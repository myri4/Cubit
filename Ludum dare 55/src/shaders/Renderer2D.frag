#pragma shader_stage(fragment)
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 1) uniform sampler2D u_Textures[];

layout(location = 0) in vec2 v_TexCoords;
layout(location = 1) in flat uint v_TexID;
layout(location = 2) in vec4 v_Color;
layout(location = 3) in float v_Fade;
layout(location = 4) in float v_Thickness;

layout(location = 0) out vec4 FragColor;

void main() 
{
    vec4 color = texture(u_Textures[v_TexID], v_TexCoords) * v_Color;

    FragColor = color;
    if (v_Thickness > 0.f) 
    {        
        float dist = 1.f - length(v_TexCoords);
        float alpha = smoothstep(0.f, v_Fade, dist) * smoothstep(v_Thickness + v_Fade, v_Thickness, dist);
        
        FragColor = v_Color;
        FragColor.a = alpha * v_Color.a;
    }

    if (v_TexID != 0) FragColor.rgb *= 0.3f;
}